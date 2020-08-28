/*
 * Copyright 2020 Brian Dodge
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "bbuf.h"
#include "butil.h"

/// \file
///

/// \brief Close a file:// file
///
/// See ::file_close_t for details
///
static int file_file_close(file_t *file)
{
    int fd = (file ? (int)(uintptr_t)file->priv : -1);
    
    if (fd >= 0)
    {
        close(fd);
    }
    file->priv = NULL;
    return 0;
}

/// \brief Read a file:// file
///
/// See ::file_read_t for details
///
static int file_file_read(file_t *file, uint8_t *buffer, size_t count)
{
    int fd = (file ? (int)(uintptr_t)file->priv : -1);

    return read(fd, (char*)buffer, count);
}

/// \brief Write a file:// file
///
/// See ::file_write_t for details
///
static int file_file_write(file_t *file, uint8_t *buffer, size_t count)
{
    int fd = (file ? (int)(uintptr_t)file->priv : -1);

    return write(fd, (char*)buffer, count);
}

/// \brief Seek in a file:// file
///
/// See ::file_seek_t for details
///
static int file_file_seek(file_t *file, uint64_t position)
{
    int fd = (file ? (int)(uintptr_t)file->priv : -1);

    file->position = lseek(fd, position, SEEK_SET);
    return 0;
}

//-----------------------------------------------------------------------------
/// \brief Setup a file:// file object
///
/// @param[in] file     - a file object with url set
/// @param[in] open_for - how to open the file, see ::open_attribute_t
///
/// @return 0 on success, non-0 on error
///
static int file_file_setup(file_t *file, open_attribute_t open_for)
{
    int fd;
    
    // setup object functions
    file->file_close    = file_file_close;
    file->file_read     = file_file_read;
    file->file_write    = file_file_write;
    file->file_seek     = file_file_seek;
    
    // setup underlying stream
    switch (open_for)
    {
    case openForRead:
        fd = open(file->url, O_RDONLY | O_CREAT, 0644);
        break;
    case openExistingForRead:
        fd = open(file->url, O_RDONLY, 0644);
        break;
    case openForWrite:
        fd = open(file->url, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        break;
    case openForAppend:
        fd = open(file->url, O_WRONLY, 0644);
        break;
    }
    if (fd < 0)
    {
        butil_log(2, "Can't open file %s\n", file->url);
        return -1;
    }
    file->priv = (void*)(uintptr_t)fd;
    return 0;
}

/// \brief Close a http:// file
///
/// See ::file_close_t for details
///
static int file_http_close(file_t *file)
{
    return 0;
}

/// \brief Read a http:// file
///
/// See ::file_read_t for details
///
static int file_http_read(file_t *file, uint8_t *buffer, size_t count)
{
    return 0;
}

/// \brief Write a http:// file
///
/// See ::file_write_t for details
///
static int file_http_write(file_t *file, uint8_t *buffer, size_t count)
{
    return 0;
}

/// \brief Seek in a http:// file
///
/// See ::file_seek_t for details
///
static int file_http_seek(file_t *file, uint64_t position)
{
    return 0;
}

//-----------------------------------------------------------------------------
/// \brief Setup a http:// file object
///
/// @param[in] file     - a file object with url set
/// @param[in] open_for - how to open the file, see ::open_attribute_t
///
/// @return 0 on success, non-0 on error
///
static int file_http_setup(file_t *file, open_attribute_t open_for)
{
    file->file_close    = file_http_close;
    file->file_read     = file_http_read;
    file->file_write    = file_http_write;
    file->file_seek     = file_http_seek;
    return 0;
}

/// \brief Get the protocol for a given url
///
/// Defaults to file:// if not specified
///
/// @param[in]  url     - url to get protocol for
/// @param[out] path    - path (filename) portion of url, for convenience. may be NULL
/// @param[in]  npath   - number of bytes in path. may be 0.

static butil_url_scheme_t file_get_scheme(const char *url, char *path, size_t npath)
{
    butil_url_scheme_t scheme;
    int result;
    
    scheme = schemeFILE;
    
    // if there is no scheme in the url, default to file://
    // (dont use butil_parse_url default)
    //
    if (!url || !strstr(url, "://"))
    {
        if (path && npath)
        {
            strncpy(path, url, npath -1);
            path[npath - 1] = '\0';
        }
        return scheme;
    }
    // break url into components to get protocol
    //
    result = butil_parse_url(
                            url,            // url
                            &scheme,        // scheme
                            NULL,           // host
                            0,              // nhost   
                            NULL,           // port
                            path,           // path
                            npath           // npath
                            );
    if (result)
    {
        return schemeFILE;
    }
    return scheme;
}

file_t *file_create(const char *url, open_attribute_t open_for)
{
    file_t *file;
    butil_url_scheme_t scheme;
    char path[MAX_PATH];
    int result;
    
    if (! url)
    {
        butil_log(2, "No url\n");
        return NULL;
    }
    file = (file_t*)malloc(sizeof(file_t));
    if (! file)
    {
        butil_log(1, "Can't alloc file\n");
        return NULL;
    }
    file->position = 0;
    result = -1;
    
    scheme = file_get_scheme(url, path, sizeof(path));

    // setup the object as appropriate for url scheme
    //
    switch (scheme)
    {
    case schemeFILE:
        strncpy(file->url, path, sizeof(file->url) - 1);
        file->url[sizeof(file->url) - 1] = '\0';    
        result = file_file_setup(file, open_for);
        break;
    /*
    case schemeFTP:
    case schemeSFTP:
        break;
    */
    case schemeDAV:
    case schemeHTTP:
    case schemeHTTPS:
        strncpy(file->url, url, sizeof(file->url) - 1);
        file->url[sizeof(file->url) - 1] = '\0';    
        result = file_file_setup(file, open_for);
        break;
    /*
    case schemeSSH:
        break;
    */
    default:
        butil_log(1, "Unsupported scheme %s for %s\n", butil_scheme_name(scheme), __FUNCTION__);
        break;
    }
    
    if (result)
    {
        butil_log(2, "Can't access url %s\n", url);
        if (file)
        {
            free(file);
        }
        return NULL;
    }
    return file;
}

void file_destroy(file_t *file)
{
    if (! file)
    {
        return;
    }
    if (file->file_close)
    {
        file->file_close(file);
    }
    free(file);
}

/// \brief Determine line-ending style from existing data
///
/// @param[in] data - data to sniff
/// @param[in] size - amount of bytes available for sniffing
///
/// @return the sniffed or guessed line ending type
///
line_ending_t file_sniff_line_endings(uint8_t *data, size_t size)
{
    int i;
    
    if (!data || !size)
    {
        return endingNONE;
    }
    for (i = 0; i < size; i++)
    {
        if (data[i] == '\n')
        {
            if (i > 0 && (data[i - 1] == '\r'))
            {
                return endingCRLF;
            }
            return endingLF;
        }
    }
    return endingNONE;
}

/// \brief Determine text encoding from start of existing data
///
/// @param[in] data - data to sniff
/// @param[in] size - amount of bytes available for sniffing
///
/// @return the sniffed or guessed text endcoding
///
text_encoding_t file_sniff_encoding(uint8_t *data, size_t size)
{
    size_t i;
    size_t n2byte_encodings;
    size_t n3byte_encodings;
    size_t n4byte_encodings;
    size_t nbad_encodings;
    size_t nutf8_encodings;
    size_t nlow_binary;
    uint8_t byte;

    if (!data || !size)
    {
        return textASCII;
    }
    if (size < 2)
    {
        return textASCII;
    }
    if(data[0] == 0xFE && data[1] == 0xFF)
    {
        if(size >= 4 && (data[2] == 0x0 && data[3] == 0x0))
        {
            return textUCS4BE; // big-endian utf-32 format
        }
        else
        {
            return textUCS2BE; // big-endian utf-16 format
        }
    }
    else if(data[0] == 0xFF && data[1] == 0xFE)
    {
        if(size >= 4 && (data[2] == 0x0 && data[3] == 0x0))
        {
            return textUCS4LE; // little-endian utf-32 (e.g. linux x86)
        }
        else
        {
            return textUCS2LE; // little-endian utf-16 (e.g. windows)
        }
    }
    else if(data[0] == 0x0 && data[1] == 0x0)
    {
        if(size >= 4 && (data[2] == 0xFE && data[3] == 0xFF))
        {
            return textUCS4BE; // big-endian utf-32 (linux ppc)
        }
        else
        {
            return textBINARY;
        }
    }
    else if(data[0] == 0xEF)
    {
        if(size >= 3 && (data[1] == 0xBB && data[2] == 0xBF))
        {
            return textUTF8;
        }
        else
        {
            return textBINARY;
        }
    }
    // look for evidence of actual utf-8 encodings
    //
    n2byte_encodings = 0;
    n3byte_encodings = 0;
    n4byte_encodings = 0;
    nbad_encodings = 0;
    nlow_binary = 0;
    
    for (i = 0; i < size - 6; i++)
    {
        byte = data[i];
        if ((byte & 0x80) == 0x80)
        {
            // high bit set, check for valid utf8 encoding
            //
            nbad_encodings++;
        }
        else if (byte < 0x20 || byte == 0x7F)
        {
            // byte is super low, if non-printing flag it
            //
            if (byte != '\r' && byte != '\n' && byte != '\t')
            {
                nlow_binary++;
            }
        }
        if ((byte & 0xE0) == 0xC0)
        {
            if ((data[i + 1] & 0xC0) == 0x80)
            {
                n2byte_encodings++;
                i+= 2;
            }
        }
        else if ((byte & 0xF0) == 0xE0)
        {
            if ((data[i + 1] & 0xC0) == 0x80)
            {
                if ((data[i + 2] & 0xC0) == 0x80)
                {
                    n3byte_encodings++;
                    i+= 3;
                }
            }
        }
        else if ((byte & 0xF8) == 0xF0)
        {
            if ((data[i + 1] & 0xC0) == 0x80)
            {
                if ((data[i + 2] & 0xC0) == 0x80)
                {
                    if ((data[i + 3] & 0xC0) == 0x80)
                    {
                        n4byte_encodings++;
                        i+= 4;
                    }
                }
            }
        }
    }
    butil_log(2, "Sniffed %du 2-byte + %u 3-byte + %u 4-byte UTF8 encodings, from %u bit-7 set bytes, and %u low-binary bytes\n",
            n2byte_encodings, n3byte_encodings, n4byte_encodings, nbad_encodings, nlow_binary);

    // look at sum of n-byte encodings and if >= high-bit-set byte sequences, assume utf-8
    //
    nutf8_encodings = n2byte_encodings + n3byte_encodings + n4byte_encodings;

    if (nutf8_encodings > 0 && nutf8_encodings >= nbad_encodings && nlow_binary == 0)
    {
        return textUTF8;
    }
    if (nlow_binary > 0 || nbad_encodings > 0)
    {
        return textBINARY;
    }
    return textASCII;
}

int file_write_BOM(file_t *file, text_encoding_t encoding)
{
    size_t len;
    uint8_t bom[8];
    int count;
    int result;
    
    if (!file)
    {
        return -1;
    }
    len = 0;
    
    switch (encoding)
    {
    case textBINARY:
    case textASCII:
    default:
        return 0;
    case textUTF8:
        bom[len++] = 0xEF;
        bom[len++] = 0xBB;
        bom[len++] = 0xBf;
        break;
    case textUCS2LE:
        bom[len++] = 0xFF;
        bom[len++] = 0xFE;
        break;
    case textUCS2BE:
        bom[len++] = 0xFE;
        bom[len++] = 0xFF;
        break;
    case textUCS4LE:
        bom[len++] = 0xFF;
        bom[len++] = 0xFE;
        bom[len++] = 0;
        bom[len++] = 0;
        break;
    case textUCS4BE:
        bom[len++] = 0xFE;
        bom[len++] = 0xFF;
        bom[len++] = 0;
        bom[len++] = 0;
        break;
    }
    result = file->file_seek(file, 0);
    if (result)
    {
        return result;
    }
    count = file->file_write(file, bom, len);
    if (count != len)
    {
        return -1;
    }
    return 0;
}

int file_info(const char *url, size_t *size, time_t *mod_time)
{
    char path[MAX_PATH];
    struct stat fstat;
    butil_url_scheme_t scheme;
    int result;

    scheme = file_get_scheme(url, path, sizeof(path));
    
    if (scheme == schemeFILE)
    {
        result = stat(path, &fstat);
        if (result)
        {
            butil_log(2, "No such path: %s\n", path);
            return -1;
        }
        if (size)
        {
            *size = fstat.st_size;
        }
        if (mod_time)
        {
            *mod_time = fstat.st_mtime;
        }
        return 0;
    }
    butil_log(1, "Scheme %s not implemented for %s\n", butil_scheme_name(scheme), __FUNCTION__);
    return -1;
}

int file_delete(const char *url)
{
    char path[MAX_PATH];
    butil_url_scheme_t scheme;
    int result;

    scheme = file_get_scheme(url, path, sizeof(path));
    
    if (scheme == schemeFILE)
    {
        result = unlink(path);
        if (result)
        {
            butil_log(2, "Unlink of %s failed\n", path);
        }
        return result;
    }
    butil_log(1, "Scheme %s not implemented for %s\n", butil_scheme_name(scheme), __FUNCTION__);
    return -1;
}

int file_move(const char *source_url, const char *destination_url)
{
    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    butil_url_scheme_t src_scheme;
    butil_url_scheme_t dst_scheme;
    int result;

    src_scheme = file_get_scheme(source_url, src_path, sizeof(src_path));
    dst_scheme = file_get_scheme(destination_url, dst_path, sizeof(dst_path));
    
    if (src_scheme == schemeFILE && dst_scheme == schemeFILE)
    {
        result = rename(src_path, dst_path);
        if (result)
        {
            butil_log(2, "Rename of %s to %s failed\n", src_path, dst_path);
        }
        return result;
    }
    butil_log(1, "Scheme %s->%s not implemented for %s\n",
           butil_scheme_name(src_scheme), butil_scheme_name(dst_scheme), __FUNCTION__);
    return -1;
}

int file_get_temp(const char *hint, char *url, const size_t size)
{
    int tmpfile;
    
    if (!url || !size)
    {
        return -1;
    }
    snprintf(url, size, "%s.XXXXXX", hint ? hint : "bfile");
    tmpfile = mkstemp(url);
    if (tmpfile < 0)
    {
        butil_log(1, "mkstmp failed in %s\n", __FUNCTION__);
        return -1;
    }
    close(tmpfile);
    return 0;
}

