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
#ifndef BFILE_H
#define BFILE_H 1

#include <stdint.h>
#include <stdbool.h>

#include "bstreamio.h"

/// \file
///

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

/// File encoding type
///
typedef enum
{
	textBINARY,
	textASCII,
	textUTF8,
	textUCS2LE,
	textUCS2BE,
	textUCS4LE,
	textUCS4BE
}
text_encoding_t;

/// File line-endings type
///
typedef enum
{
	endingNONE,			///< there are no line endings
	endingLF,			///< LF only (Unix style)
	endingCRLF			///< CR-LF (DOS style)
}
line_ending_t;

/// Open Attribute - how to open a file
///
typedef enum
{
	openForRead,			///< open existing file for reading only, failing if file doesn't already exist
	openForWrite,			///< open for writing only, and remove any existing content, creating if it doesn't exist
	openForAppend			///< open existing file for appending (write) leaving any existing content
}
open_attribute_t;

struct tag_file;

/// File Close function
///
/// \brief Closes a file
///
/// @param[in] file          - file to close, as returned from ::file_create
/// @return < 0 on error
///
typedef int (*file_close_t)(struct tag_file *file);

/// File Read function
///
/// \brief Reads bytes from an open file handle
///
/// @param[in] file          - file to read as returned from ::file_create
/// @param[in] buffer        - buffer to read into
/// @param[in] count         - number of bytes to read (max)
///
/// @return number of bytes read, or < 0 for errors
///
typedef int (*file_read_t)(struct tag_file *file, uint8_t *buffer, size_t count);

/// File Write function
///
/// \brief Writes bytes to an open file handle
///
/// @param[in] file          - file to write as returned from ::file_create
/// @param[in] buffer        - buffer to write from
/// @param[in] count         - number of bytes to write (max)
///
/// @return number of bytes written, or < 0 for errors. For non-blocking
/// files/streams the write might not be complete and remaining data
/// should be written until it is all consumed
///
typedef int (*file_write_t)(struct tag_file *file, uint8_t *buffer, size_t count);

/// File Seek function
///
/// \brief Position a file at a specific offset from the start of file
///
/// @param[in] file          - file to seek in as returned from ::file_create
/// @param[in] position      - offset from starting byte to seek to
///
/// @return < 0 on error, 0 if not ready, and > 0 if ready (or closed)
///
typedef int (*file_seek_t)(struct tag_file *file, uint64_t position);

/// File - an object that provides methods for open/read/write/close/delete/rename
///        to access file data. 
///
typedef struct tag_file
{
	// public
	char            url[MAX_PATH];		///< file/stream's name
	file_close_t	file_close;			///< function to close
	file_read_t		file_read;			///< function to read
	file_write_t	file_write;			///< function to write
	file_seek_t		file_seek;			///< function to seek
	// private
	uint64_t		position;			///< current position in file (seek)
	void           *priv;				///< per-object private context
}
file_t;

/// File open credential callback function type
///
/// \brief Fetch credentials from caller for file create that needs them
///
/// @param[in]     url			- url credentials needed for
/// @param[in/out] username		- user name buffer for caller to fill in
/// @param[in]     nusername	- size of user name buffer in bytes
/// @param[in/put] password 	- password buffer for caller to fill in
/// @param[in]     npassword	- size of password buffer in bytes
///
/// @return < 0 on error, 0 if not ready, and > 0 if ready (or closed)
///
typedef int (*credential_callback_t)(const char *url, char *username, size_t nusername, char *password, size_t npassword);

/// \brief Create a file object from a url
///
/// @param[in] url - url of file to open (if no protocol in url, defaults to file://)
/// @param[in] open_for - how to open the url, see ::open_attribute_t for more info
/// @return a 
///
file_t *file_create(const char *url, const open_attribute_t open_for);

/// \brief Create a file object from a url with credentials, similar to ::file_create 
///
/// @param[in] url 					- url of file to open (if no protocol in url, defaults to file://)
/// @param[in] open_for 			- how to open the url, see ::open_attribute_t for more info
/// @param[in] credential_callback	- credential fetching function, see ::user_credentials_t for more into
/// @return a 
///
file_t *file_create_with_credentials(const char *url, const open_attribute_t open_for, credential_callback_t credential_callback);

/// \brief Destroy a file that was created with ::file_create
///
/// @param[in] file		- file to destroy
///
void file_destroy(file_t *file);

/// \brief Determine line-ending style from existing data
///
/// @param[in] data - data to sniff
/// @param[in] size - amount of bytes available for sniffing
///
/// @return the sniffed or guessed line ending type
///
line_ending_t file_sniff_line_endings(uint8_t *data, size_t size);

/// \brief Determine text encoding from start of existing data
///
/// @param[in] data - data to sniff
/// @param[in] size - amount of bytes available for sniffing
///
/// @return the sniffed or guessed text endcoding
///
text_encoding_t file_sniff_encoding(uint8_t *data, size_t size);

	/// \brief Write a Byte Order Mark (BOM) at the top of the file
///
/// @param[in] file     - file to insert BOM
/// @param[in] encoding - encoding to write BOM of
///
/// @return 0 on success
///
int file_write_BOM(file_t *file, text_encoding_t encoding);

/// \brief Get info about a file
///
/// @param[in]   url  		- url of file to get info for
/// @param[out]  size     	- gets size of file, in bytes
/// @param[out]  mod_time 	- gets last modification time of file in POSIX seconds
///
/// @return 0 on success, non-0 if url doesn't exist
///
int file_info(const char *url, size_t *size, time_t *mod_time);

/// \brief Delete an actual file
///
/// @param[in] url - url of file to delete
/// 
/// @return 0 on success
///
int file_delete(const char *url);

/// \brief Move or Rename a file
///
/// @param[in] source_url 		- url of exising file
/// @param[in] destination_url	- url of where to move file
///
/// @return 0 on success
///
int file_move(const char *source_url, const char *destination_url);

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
int file_get_temp(const char *hint, char *url, const size_t size);

#endif
