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
#ifndef BBUF_H
#define BBUF_H 1

#include <stdint.h>
#include <stdbool.h>
#include "bline.h"
#include "bfile.h"
#include "bundo.h"

/// \file
///

/// Default size of memory backing a buffer's data cache
#define BUFFER_DEFAULT_VBUF_SIZE	(8*1024*1024) /* 8Mb */

/// Buffer - represents the contents of a file
///
typedef struct tag_buffer
{
	// public
	char			name[MAX_PATH];		///< name of buffer (typically file name)
	text_encoding_t original_encoding;	///< original text encoding
	line_ending_t   original_lineends;	///< original line endings
	file_t		   *file;				///< the file/stream which is the source/destination for buffer data
	line_t	 	   *lines;				///< list of lines in the file
	size_t			line_count;			///< cache of line count
	line_t		   *curr_line;			///< current line, for performance
	size_t			curr_linenum;		///< line number at current line	
	// protected
	undorec_t	   *undos;				///< list of undos
	// private
	char	 	   *vbuf;				///< buffer of file data "around" current line
	size_t			vbuf_size;			///< how large vbuf is in bytes
	bool			vbuf_alloced;		///< set true if vbuf is owned by this object
	uint64_t		vbuf_offset;		///< offset in file where vbuf starts
	size_t			vbuf_count;			///< count of bytes in vbuf currently
	size_t			vbuf_tail;			///< read-index into vbuf
	uint8_t        *sandbox;			///< scratch buffer
	size_t          sandbox_size;		///< allocated size of scratch buffer
	size_t			sandbox_count;		///< bytes valid in sandbox
}
buffer_t;

/// \brief Create a buffer
///
/// @param[in] name 	- name to give the buffer. if NULL, the file's name will be used
/// @param[in] file 	- the file/stream object to use to back the buffer
/// @param[in] vbuf	    - an initial buffer of content, or, an initial scratch buffer, may be NULL
/// @param[in] vbufsize	- how many bytes to buffer near current line, 0 means "default". if vbuf
///                       is supplied, it should be at least this size in bytes
/// @return the allocated buffer object, or NULL on error. The returned buffer
/// should be freed using ::buffer_destroy when it is no longer needed
///
buffer_t *buffer_create(const char *name, file_t *file, uint8_t *vbuf, size_t vbufsize);

/// \brief Destroy a buffer
///
/// @param[in] buffer - the buffer to destroy, which should have been created with ::buffer_create
///
void buffer_destroy(buffer_t *buffer);

/// \brief Read a buffer
///
/// Reads the contents of the buffer's file into the buffer's line structure
/// destroying any previously existing structure and undo information
///
/// @return < 0 on error, 0 on success
///
int buffer_read(buffer_t *buffer);

/// \brief Write a buffer
///
/// Writes the contents of the buffer's line structure from the buffer's file
/// into the supplied output file which can NOT be the same as the buffer's file
/// since the buffer's lines might point to locations in the buffer's file
///
/// Leaves the line and undo information intact in the buffer 
///
/// @param[in]  buffer   - buffer to write
/// @param[in]  outfile	 - file to write to
/// @param[in]  encoding - text encoding to use for output file
///
/// @return < 0 on error, 0 on success
///
int buffer_write(buffer_t *buffer, file_t *outfile, text_encoding_t encoding);

/// \brief Get a pointer to a line's data
///
/// The pointer returned could be either the line's contents in memory
/// or the contents in vbuf, or in some other buffer and are NOT
/// null terminated. The pointer is to the RAW file data and is therefore
/// encoded with whichever file encoding is in use for the buffer's file
///
/// @param[in]  buffer  - buffer to get line in
/// @param[in]  line	- line number to edit, 0-based
/// @param[out] content	- pointer to line's contents in edit buffer
/// @param[out] length	- length of line data in bytes
/// 
/// @return 0 on success
///
int buffer_get_line_content(buffer_t *buffer, size_t line, uint8_t **content, size_t *length);

/// \brief Move a buffer line into the sandbox decoding any text encoding
///
/// @param[in] buffer - buffer to get line from
/// @param[in] line   - line number (0 based) to sandbox
/// @param[out] text  - the line's text in utf-8 encoding
/// @param[out] length	- length of text in bytes
///
/// @return 0 on success
///
int buffer_edit_line(buffer_t *buffer, size_t line, char **text, size_t *length);

#endif
