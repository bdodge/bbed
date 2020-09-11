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
#ifndef BFILE_FILE_H
#define BFILE_FILE_H 1

#include <stdint.h>
#include <stdbool.h>

#include "bfile.h"
#include "bstreamio.h"

/// \file
///


//-----------------------------------------------------------------------------
/// \brief Setup a file:// file object
///
/// @param[in] file     - a file object with url set
/// @param[in] open_for - how to open the file, see ::open_attribute_t
///
/// @return 0 on success, non-0 on error
///
int file_file_setup(file_t *file, open_attribute_t open_for, credential_callback_t credential_callback);

#endif
