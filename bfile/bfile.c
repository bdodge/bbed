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
#include "bfile.h"
#include "bfile_file.h"
#include "bfile_http.h"
#include "bfile_ftp.h"
#include "butil.h"

/// \file
///

butil_url_scheme_t file_get_scheme(const char *url, char *path, size_t npath)
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

file_t *file_create_with_credentials(const char *url, open_attribute_t open_for, credential_callback_t credential_callback)
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
        result = file_file_setup(file, open_for, credential_callback);
        break;

    case schemeFTP:
    case schemeSFTP:
        strncpy(file->url, url, sizeof(file->url) - 1);
        file->url[sizeof(file->url) - 1] = '\0';        
        result = file_ftp_setup(file, open_for, credential_callback);
        break;

    case schemeDAV:
    case schemeHTTP:
    case schemeHTTPS:
        strncpy(file->url, url, sizeof(file->url) - 1);
        file->url[sizeof(file->url) - 1] = '\0';        
        result = file_http_setup(file, open_for, credential_callback);
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

file_t *file_create(const char *url, open_attribute_t open_for)
{
    return file_create_with_credentials(url, open_for, NULL);
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


