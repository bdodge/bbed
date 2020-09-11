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
#ifndef BFILESYS_H
#define BFILESYS_H 1

#include <stdint.h>
#include <stdio.h>

/// \file
///

/// \brief Get info about a file
///
/// @param[in]   url  		- url of file to get info for
/// @param[out]  size     	- gets size of file, in bytes
/// @param[out]  mod_time 	- gets last modification time of file in POSIX seconds
///
/// @return 0 on success, non-0 if url doesn't exist
///
int filesys_info(const char *url, size_t *size, time_t *mod_time);

/// \brief Delete an actual file
///
/// @param[in] url - url of file to delete
/// 
/// @return 0 on success
///
int filesys_delete(const char *url);

/// \brief Move or Rename a file
///
/// @param[in] source_url 		- url of exising file
/// @param[in] destination_url	- url of where to move file
///
/// @return 0 on success
///
int filesys_move(const char *source_url, const char *destination_url);

/// \brief Get a temporary file path
///
/// The file path for a temporary file is generated and the file is 
/// created so opening it as an existing file should work 
///
/// @param[in]  hint	- use this string some place in the path if possible, may be NULL
/// @param[out] url		- buffer to receive url of temporary file
/// @param[in]  size  	- sizeof url buffer in bytes
///
/// @return 0 on success, non-0 on error (can't create file)
///
int filesys_get_temp(const char *hint, char *url, const size_t size);

#endif
