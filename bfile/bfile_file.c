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
#include "bfile_file.h"
#include "butil.h"

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

int file_file_setup(file_t *file, open_attribute_t open_for, credential_callback_t credential_callback)
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

