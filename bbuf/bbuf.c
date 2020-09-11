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

/// \brief Get a single unicode character from current positon of buffer
///
/// Reads enough characters from the buffers underlying file to get one
/// character in the file's encoding, reading in new data as required
///
/// @param[in] buffer   - buffer to read from
/// @param[in] encoding - text encoding the data is 
/// @param[out] ucode   - unicode character read
///
/// @return 0 on success, -1 on failure (end of file)
///
static int buffer_get_unicode_char(buffer_t *buffer, text_encoding_t encoding, unicode_char_t *ucode)
{
    int result;
    size_t avail;
    size_t used;
    bool at_eof;
    uint32_t b;
    uint32_t c;
    uint16_t u;
    
    // how many bytes are buffered?
    //
    avail = buffer->vbuf_count - buffer->vbuf_tail;
    
    // if less than 8 (utf-8 encoding may need a bunch) re-buffer
    //
    if (avail < 8)
    {
        // move remnants to start of vbuf
        //
        memmove(buffer->vbuf, buffer->vbuf + buffer->vbuf_tail, avail);
        
        // set vbuf offset to where that data was
        //
        buffer->vbuf_offset += buffer->vbuf_tail;
        
        // set tail to start of data and set count avail
        //
        buffer->vbuf_tail = 0;
        buffer->vbuf_count = avail;
        
        // read more in
        //
        result = buffer->file->file_read(buffer->file, buffer->vbuf + buffer->vbuf_tail, buffer->vbuf_size - buffer->vbuf_count);
        if (result <= 0)
        {
            at_eof = true;
        }
        else
        {
            at_eof = false;
            buffer->vbuf_count += result;
        }
    }
    if (avail == 0)
    {
        // EOF
        return -1;
    }
    // decode byte(s) to get a single char
    //
    switch (encoding)
    {
    case textBINARY:
    case textASCII:
        u = (uint16_t)buffer->vbuf[buffer->vbuf_tail++];
        break;

    case textUTF8:
        used = butil_utf8_decode(buffer->vbuf + buffer->vbuf_tail, avail, &b);
        avail -= used;
        buffer->vbuf_tail += used;
        u = (uint16_t)b;
        break;

    case textUCS2LE:
        b = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;
        b <<= 8;
        if (avail < 1)
        {
            // dangling?
            return -1;
        }                    
        c = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;                    
        b |= c;
        u = (uint16_t)b;
        break;

    case textUCS2BE:
        b = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;
        if (avail < 1)
        {
            // dangling?
            return -1;
        }                    
        c = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;                    
        b |= (c << 8);
        u = (uint16_t)b;
        break;

    case textUCS4LE:
        b = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;
        b <<= 8;
        if (avail < 1)
        {
            // dangling?
            return -1;
        }                    
        c = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;                    
        b |= c;
        if (avail < 2)
        {
            // dangling?
            return -1;
        }
        buffer->vbuf_tail += 2;
        avail -= 2;
        u = (uint16_t)b;
        break;

    case textUCS4BE:
        if (avail < 4)
        {
            return -1;
        }
        buffer->vbuf_tail += 2;
        avail-= 2;
        b = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;
        if (avail < 1)
        {
            // dangling?
            return -1;
        }                    
        c = (uint32_t)buffer->vbuf[buffer->vbuf_tail++];
        avail--;                    
        b |= (c << 8);
        u = (uint16_t)b;
        break;
    }
    if (ucode)
    {
        *ucode = u;
    }
    return 0;
}

/// \brief Allocate sandbox to fit a line
///
/// @param[in] buffer - buffer to allocate sandbox in
/// @param[in] size   - size needed
///
/// @return 0 on success
///
static int buffer_size_sandbox(buffer_t *buffer, size_t size)
{
    if (size > buffer->sandbox_size || buffer->sandbox == NULL)
    {
        size_t newsize;
        
        // make new size a bunch bigger and align to 32 bytes
        //
        newsize = size * 2;
        if (newsize < size)
        {
            // whoa.. size is already near max.. tone it down
            //
            newsize = size + 64;
        }
        if (newsize < size)
        {
            butil_log(1, "%s: size of line: %u is much too large\n", __FUNCTION__, size);
            return -1;
        }
        newsize += 31;
        newsize &= ~31;
        
        if (buffer->sandbox)
        {
            free(buffer->sandbox);
        }
        buffer->sandbox = (uint8_t*)malloc(newsize);
        if (! buffer->sandbox)
        {
            butil_log(0, "%s: Can't alloc sandbox\n", __FUNCTION__);
            buffer->sandbox_size = 0;
            return -1;
        }
        buffer->sandbox_size = newsize;
    }
    return 0;
}

/// \brief Move to line in buffer
///
/// @param[in] buffer - buffer to allocate sandbox in
/// @param[in] line   - line (0 based) to move to
///
/// @return 0 on success
///
static int buffer_select_line(buffer_t *buffer, size_t line)
{
    if (!buffer || !buffer->lines)
    {
        return -1;
    }
    // find the line in the buffer
    //
    if (line > buffer->line_count)
    {
       return -1;
    }
    if (! buffer->curr_line)
    {
        buffer->curr_line = buffer->lines;
        buffer->curr_linenum = 0;
    }
    while (buffer->curr_linenum > line)
    {
        if (!buffer->curr_line->prev)
        {
            // can't happen!
            butil_log(0, "No prev for line %d\n", buffer->curr_linenum);
            return -1;
        }
        buffer->curr_line = buffer->curr_line->prev;
        buffer->curr_linenum--;
    }
    while (buffer->curr_linenum < line)
    {
        if (!buffer->curr_line->next)
        {
            return -1;
        }
        buffer->curr_line = buffer->curr_line->next;
        buffer->curr_linenum++;
    }
    return 0;
}

buffer_t *buffer_create(const char *name, file_t *file, uint8_t *vbuf, size_t vbufsize)
{
    buffer_t *buffer;
    
    buffer = (buffer_t*)malloc(sizeof(buffer_t));
    if (! buffer)
    {
        butil_log(1, "Can't alloc buffer\n");
        return NULL;
    }
    memset(buffer, 0, sizeof(buffer));
    
    if (! vbufsize)
    {
        vbufsize = BUFFER_DEFAULT_VBUF_SIZE;
    }
    buffer->vbuf_size = vbufsize;
    
    if (! vbuf)
    {
        vbuf = (uint8_t*)malloc(vbufsize);
        if (! vbuf)
        {
            butil_log(1, "Can't alloc vbuf for buffer\n");
            free(buffer);
            return NULL;
        }
        buffer->vbuf_alloced = true;
    }
    buffer->vbuf = vbuf;
    buffer->file = file;
    buffer->vbuf_offset = 0;
    buffer->vbuf_count  = 0;
    buffer->curr_line = NULL;
    buffer->curr_linenum = 0;
    buffer->lines = 0;
    buffer->undos = NULL;
    
    buffer->sandbox = NULL;
    buffer->sandbox_size = 0;
    buffer->sandbox_count = 0;
    
    return buffer;
}

void buffer_destroy(buffer_t *buffer)
{
    if (! buffer)
    {
        return;
    }
    if (buffer->vbuf && buffer->vbuf_alloced)
    {
        free(buffer->vbuf);
    }
    if (buffer->sandbox)
    {
        free(buffer->sandbox);
    }
}

int buffer_read(buffer_t *buffer)
{
    int result;
    int fudge;
    uint64_t line_offset;
    line_t *line;
    unicode_char_t ucode;
    
    if (!buffer || !buffer->file || !buffer->vbuf)
    {
        butil_log(1, "%s: Bad parameter\n", __FUNCTION__);
        return -1;
    }
    // make sure at start of file
    //
    result = buffer->file->file_seek(buffer->file, 0);
    if (result)
    {
        return result;
    }
    // read a buffer's worth and sniff file encoding
    //
    result = buffer->file->file_read(buffer->file, buffer->vbuf, buffer->vbuf_size);
    if (result < 0)
    {
        butil_log(2, "%s: Can't read file\n", __FUNCTION__);
        return result;
    }
    buffer->vbuf_tail = 0;
    buffer->vbuf_offset = 0;
    buffer->vbuf_count = result;
    buffer->original_encoding = file_sniff_encoding(buffer->vbuf, buffer->vbuf_count);
    buffer->original_lineends = file_sniff_line_endings(buffer->vbuf, buffer->vbuf_count);

    // set offset past any file byte-order-mark header
    //
    switch (buffer->original_encoding)
    {
    case textBINARY:
        fudge = 0;
        break;
    case textASCII:
        fudge = 0;
        break;
    case textUTF8:
        fudge = ((uint8_t)buffer->vbuf[0] == 0xef) ? 3 : 0;
        break;
    case textUCS2LE:
        fudge = 2;
        break;
    case textUCS2BE:
        fudge = 2;
        break;
    case textUCS4LE:
        fudge = 4;
        break;
    case textUCS4BE:
        fudge = 4;
        break;
    default:
        fudge = 0;
        break;
    }
    buffer->vbuf_tail = fudge;
    line_offset = buffer->vbuf_offset + buffer->vbuf_tail;
    line = NULL;
    buffer->curr_line = NULL;
    buffer->curr_linenum = 0;
    
    // iterate over entire file contents finding locations of LF (newline) and set
    // line position in file, creating a line for each found line
    //
    do
    {
        // parse chars forward until a newline is hit
        //
        result = buffer_get_unicode_char(buffer, buffer->original_encoding, &ucode);
        if (result)
        {
            break;
        }
        if (ucode == '\n')
        {
            // create a line at this position
            //
            line = line_create_from_location(line_offset, buffer->vbuf_offset + buffer->vbuf_tail - line_offset);
            if (!line)
            {
                butil_log(1, "%s: Can't make line\n", __FUNCTION__);
                break;
            }
            else
            {
                line->prev = buffer->curr_line;
                line->next = NULL;
                if (buffer->curr_line)
                {
                    buffer->curr_line->next = line;
                }
                else
                {
                    buffer->lines = line;
                }
                buffer->curr_line = line;
                buffer->curr_linenum++;
            }
            line_offset = buffer->vbuf_offset + buffer->vbuf_tail;
            line = NULL;
        }
    }
    while (!result);

    if ((buffer->vbuf_offset + buffer->vbuf_tail) > line_offset)
    {
        // non-terminated line at end, create a line at this position
        //
        line = line_create_from_location(line_offset, buffer->vbuf_offset + buffer->vbuf_tail - line_offset);
        if (!line)
        {
            butil_log(1, "%s: Can't make line\n", __FUNCTION__);
        }
        else
        {
            line->prev = buffer->curr_line;
            line->next = NULL;
            if (buffer->curr_line)
            {
                buffer->curr_line->next = line;
            }
            else
            {
                buffer->lines = line;
            }
            buffer->curr_line = line;
            buffer->curr_linenum++;
        }
    }
    buffer->line_count = buffer->curr_linenum;
    butil_log(3, "%s: %d lines from %d bytes\n", __FUNCTION__, buffer->line_count, buffer->vbuf_offset + buffer->vbuf_count);

    // leave with line at top
    buffer->curr_line = buffer->lines;
    buffer->curr_linenum = 0;
    return 0;
}

int buffer_write(buffer_t *buffer, file_t *outfile, text_encoding_t encoding)
{
    uint8_t *linedata;
    char    *linetext;
    size_t   linenum;
    size_t   length;
    size_t   count;
    size_t   used;
    size_t   index;
    size_t   outdex;
    uint32_t unicode;
    int      result;
    
    if (!buffer || !buffer->file || !outfile)
    {
        butil_log(1, "%s: Bad parameter\n", __FUNCTION__);
        return -1;
    }
    result = file_write_BOM(outfile, encoding);
    
    if (buffer->original_encoding == encoding)
    {
        // special case writing file in same encoding to avoid double buffering
        //
        for (linenum = 0; linenum < buffer->line_count; linenum++)
        {
            result = buffer_get_line_content(buffer, linenum, &linedata, &length);
            if (result)
            {
                // assume buffer line count is wrong?  or error?
                break;
            }
            // if line is in memory, it is in utf-8 (native) format, so transcode
            // to output format as needed, otherwise directly write file data
            //
            if (buffer->curr_line->location == lineInMemory)
            {
                switch (encoding)
                {
                case textBINARY:
                case textASCII:
                case textUTF8:
                default:
                    // use line content directly in memory with no transcoding
                    break;
                case textUCS2LE:
                case textUCS2BE:
                case textUCS4LE:
                case textUCS4BE:
                    // trancode via sandbox from utf8 to encoding
                    result = buffer_size_sandbox(buffer, length * 4 + 4);
                    if (result)
                    {
                        return result;
                    }
                    index = 0;
                    outdex = 0;
                    
                    while (index < length)
                    {
                        used = butil_utf8_decode(linedata + index, length - index, &unicode);
                        index += used;
                        
                        switch (encoding)
                        {
                        default:
                        case textUCS2LE:
                            buffer->sandbox[outdex++] = unicode & 0xFF;
                            buffer->sandbox[outdex++] = (unicode >> 8) & 0xFF;
                            break;
                        case textUCS2BE:
                            buffer->sandbox[outdex++] = (unicode >> 8) & 0xFF;
                            buffer->sandbox[outdex++] = unicode & 0xFF;
                            break;
                        case textUCS4LE:
                            buffer->sandbox[outdex++] = unicode & 0xFF;
                            buffer->sandbox[outdex++] = (unicode >> 8) & 0xFF;
                            buffer->sandbox[outdex++] = (unicode >> 16) & 0xFF;
                            buffer->sandbox[outdex++] = (unicode >> 24) & 0xFF;
                            break;
                        case textUCS4BE:
                            buffer->sandbox[outdex++] = (unicode >> 24) & 0xFF;
                            buffer->sandbox[outdex++] = (unicode >> 16) & 0xFF;
                            buffer->sandbox[outdex++] = (unicode >> 8) & 0xFF;
                            buffer->sandbox[outdex++] = unicode & 0xFF;
                            break;
                        }
                    }
                    linedata = buffer->sandbox;
                    length = outdex;
                    break;
                }
            }
            count = outfile->file_write(outfile, linedata, length);
            if (count != length)
            {
                butil_log(1, "%s: Can't write output file\n", __FUNCTION__);
                break;
            }
        }
    }
    else
    {
        uint8_t *newline = NULL;
        size_t newline_size = 0;
        size_t newsize;
        
        // general case where we always need to transcode
        //
        for (linenum = 0; linenum < buffer->line_count; linenum++)
        {
            // get line utf8 encoded in memory
            //
            result = buffer_edit_line(buffer, linenum, &linetext, &length);
            if (result)
            {
                // assume buffer line count is wrong?  or error?
                break;
            }
            switch (encoding)
            {
            case textBINARY:
            case textASCII:
            case textUTF8:
            default:
                // use line content directly in memory with no transcoding
                newline == NULL;
                break;
            case textUCS2LE:
            case textUCS2BE:
            case textUCS4LE:
            case textUCS4BE:
                index = 0;
                outdex = 0;
                newsize = length * 4 + 4;
                if (newsize > newline_size || !newline)
                {
                    newline_size = newsize * 2;
                    if (newline)
                    {
                        free(newline);
                    }
                    if (newline_size < newsize)
                    {
                        // integer overflow? rut roh
                        butil_log(1, "%s: line size way too large\n");
                        newline_size = newsize;
                    }
                    newline = (uint8_t*)malloc(newline_size);
                }
                if (!newline)
                {
                    return -1;
                }
                while (index < length)
                {
                    used = butil_utf8_decode(linetext + index, length - index, &unicode);
                    index += used;
                    
                    switch (encoding)
                    {
                    default:
                    case textUCS2LE:
                        newline[outdex++] = unicode & 0xFF;
                        newline[outdex++] = (unicode >> 8) & 0xFF;
                        break;
                    case textUCS2BE:
                        newline[outdex++] = (unicode >> 8) & 0xFF;
                        newline[outdex++] = unicode & 0xFF;
                        break;
                    case textUCS4LE:
                        newline[outdex++] = unicode & 0xFF;
                        newline[outdex++] = (unicode >> 8) & 0xFF;
                        newline[outdex++] = (unicode >> 16) & 0xFF;
                        newline[outdex++] = (unicode >> 24) & 0xFF;
                        break;
                    case textUCS4BE:
                        newline[outdex++] = (unicode >> 24) & 0xFF;
                        newline[outdex++] = (unicode >> 16) & 0xFF;
                        newline[outdex++] = (unicode >> 8) & 0xFF;
                        newline[outdex++] = unicode & 0xFF;
                        break;
                    }
                }
                linedata = newline;
                length = outdex;
                break;
            }
            count = outfile->file_write(outfile, linedata, length);
            if (count != length)
            {
                butil_log(1, "%s: Can't write output file\n", __FUNCTION__);
                break;
            }
        }
        if (newline)
        {
            free(newline);
        }
    }
    return 0;
}

int buffer_get_line_content(buffer_t *buffer, size_t line, uint8_t **content, size_t *length)
{
    uint64_t offset;
    int result;
    
    if (!content || !length)
    {
        return -1;
    }
    *content = "";
    *length = 0;

    if (!buffer || !buffer->lines)
    {
        return -1;
    }
    result = buffer_select_line(buffer, line);
    if (result)
    {
        return result;
    }
    // if line is already in memory, all set
    //
    if (buffer->curr_line->location == lineInMemory)
    {
        *content = buffer->curr_line->position.data;
        *length = buffer->curr_line->length;
        return 0;
    }
    // check that the line is wholly inside the buffer's vbuf and if not
    // re-buffer around the region
    //
    if (
            (buffer->curr_line->position.offset < buffer->vbuf_offset)
        ||  ((buffer->curr_line->position.offset + buffer->curr_line->length)> (buffer->vbuf_offset + buffer->vbuf_count))
    )
    {
        size_t margin;
        int result;
        
        // center position in vbuf
        //
        if (buffer->curr_line->length > buffer->vbuf_size)
        {
            butil_log(1, "%s: Line of %d can't fit in vbuf of size %d\n", __FUNCTION__, 
                buffer->curr_line->length, buffer->vbuf_size);
            return -1;
        }
        margin = (buffer->vbuf_size - buffer->curr_line->length) / 2;
        offset = buffer->curr_line->position.offset;
        if (offset > margin)
        {
            offset -= margin;
        }
        offset &= ~0x1F; // align to 32 bytes
        
        // reposition file at offset and read a chunk
        //
        result = buffer->file->file_seek(buffer->file, offset);
        if (result)
        {
            butil_log(1, "%s: Can't reposition in file\n", __FUNCTION__);
            return -1;
        }
        buffer->vbuf_offset = offset;

        result = buffer->file->file_read(buffer->file, buffer->vbuf, buffer->vbuf_size);
        if (result <= 0)
        {
            butil_log(1, "%s: Can't read from file\n", __FUNCTION__);
            return -1;
        }
        else
        {
            buffer->vbuf_count = result;
        }
    }
    
    // location in vbuf where line starts
    //
    offset = buffer->curr_line->position.offset - buffer->vbuf_offset;

    // for convenience, set read tail to start of curr_line content in vbuf
    buffer->vbuf_tail = offset;
    
    *content = buffer->vbuf + offset;
    *length = buffer->curr_line->length;
    return 0;
}

int buffer_edit_line(buffer_t *buffer, size_t line, char **text, size_t *length)
{
    uint8_t *content;
    uint8_t *pdest;
    size_t rawlength;
    size_t i;
    uint32_t ucode;
    int result;
    
    if (text)
    {
        *text = "";
    }
    if (length)
    {
        *length = 0;
    }
    buffer->sandbox_count = 0;
    
    // get raw bytes in file
    //
    result = buffer_get_line_content(buffer, line, &content, &rawlength);
    if (result)
    {
        return result;
    }
    // check sandbox size, need to maybe grow times 4 for utf-8 encoding
    //
    result = buffer_size_sandbox(buffer, rawlength * 4 + 4);
    if (result)
    {
        return result;
    }
    switch (buffer->original_encoding)
    {
    case textBINARY:
    case textASCII:
    case textUTF8:
    default:
        memcpy(buffer->sandbox, content, rawlength);
        pdest = buffer->sandbox + rawlength;
        break;
    case textUCS2LE:
        for (i = 0, pdest = buffer->sandbox; i < rawlength; i+= 2)
        {
            ucode = ((uint32_t)content[i]) | ((uint32_t)content[i + 1] << 8);
            butil_utf8_encode(ucode, pdest);
            while (*pdest)
            {
                pdest++;
            }
        }
        break;
    case textUCS2BE:
        for (i = 0, pdest = buffer->sandbox; i < rawlength; i+= 2)
        {
            ucode = ((uint32_t)content[i] << 8) | ((uint32_t)content[i + 1]);
            butil_utf8_encode(ucode, pdest);
            while (*pdest)
            {
                pdest++;
            }
        }
        break;
    case textUCS4LE:
        for (i = 0, pdest = buffer->sandbox; i < rawlength; i+= 4)
        {
            ucode  = ((uint32_t)content[i]) | ((uint32_t)content[i + 1] << 8);
            ucode |= ((uint32_t)content[i + 2] << 16) | ((uint32_t)content[i + 3] << 24);
            
            butil_utf8_encode(ucode, pdest);
            while (*pdest)
            {
                pdest++;
            }
        }
        break;
    case textUCS4BE:
        for (i = 0, pdest = buffer->sandbox; i < rawlength; i+= 4)
        {
            ucode  = ((uint32_t)content[i] << 24) | ((uint32_t)content[i + 1] << 16);
            ucode |= ((uint32_t)content[i + 2] << 8) | ((uint32_t)content[i + 3]);
            butil_utf8_encode(ucode, pdest);
            while (*pdest)
            {
                pdest++;
            }
        }
        break;
    }
    // null terminate the sandbox and remember length in bytes
    //
    *pdest = '\0';
    buffer->sandbox_count = pdest - buffer->sandbox;
    
    if (text)
    {
        *text = (char*)buffer->sandbox;
    }
    if (length)
    {
        *length = buffer->sandbox_count;
    }
    return 0;
}

