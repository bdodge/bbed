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

/// \file
///

line_t *line_create_from_location(uint64_t offset, size_t length)
{
    line_t *line;
    
    line = (line_t*)malloc(sizeof(line_t));
    if (!line)
    {
        return NULL;
    }
    line->location = lineInFile;
    line->position.offset = offset;
    line->length = length;
    return line;
}

line_t *line_create_from_data(uint8_t *data, size_t length, bool copy)
{
    line_t *line;
    
    line = (line_t*)malloc(sizeof(line_t));
    if (!line)
    {
        return NULL;
    }
    line->location = lineInMemory;
    if (copy)
    {
        uint8_t *newdata = (uint8_t*)malloc(length + 1);
        if (! newdata)
        {
            free(line);
            return NULL;
        }
        memcpy(newdata, data, length);
        newdata[length] = '\0';
        data = newdata;
    }
    line->position.data = data;
    line->length = length;
    return line;
}


