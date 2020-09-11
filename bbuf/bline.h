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
#ifndef BLINE_H
#define BLINE_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/// \file
///

/// \brief Attributes of a line
///
#define	lineStartsSpanningComment	0x0001	///< the line starts a spanning comment block
#define lineEndsSpanningComment		0x0002	///< the line ends a spanning comment block

/// Attributes a line can have
///
typedef uint32_t line_attribute_t;

/// Unicode character type (dont use underlying system's type)
///
typedef uint16_t unicode_char_t;

/// Line - represents a line in a file
///
/// The line exists in either a file or in memory
///   If in a file, the location is specified
///   by an offset into the file
///   If in memory, the location is a pointer to the allocated buffer
///
typedef struct tag_line
{
	enum
	{
		lineInFile,			///< the line is in the file
		lineInMemory		///< the line has been buffered in memory
	}
	location;				///< the line's location
	
	union
	{
		uint64_t offset;	///< offset into the file, if location is lineInFile
		char   	*data;		///< buffered line data, if location is lineInMemory
	}
	position;				///< the lines position in its location

	/// Line attributes
	line_attribute_t attributes;
	
	/// Line length, in bytes
	size_t length;
	
	struct tag_line *prev;	///< previous line in list
	struct tag_line *next;	///< next line in list
}
line_t;

/// \brief Create a line with data in a file location
///
/// @param[in] offset 	- location of line as offset from start
/// @param[in] length 	- length of line in bytes
///
/// @return the new line, or NULL if no memory
///
line_t *line_create_from_location(uint64_t offset, size_t length);

/// \brief Create a line with data in memory
///
/// @param[in] data   	- data for line
/// @param[in] length 	- length of data in bytes
/// @param[in] copy 	- allocate and copy the data if true, take ownership of data if false
///
/// @return the new line, or NULL if no memory
///
line_t *line_create_from_data(uint8_t *data, size_t length, bool copy);

#endif
