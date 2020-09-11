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
#include "bfile_ftp.h"
#include "bfile.h"
#include "bfilesys.h"
#include "bftp.h"
#include "butil.h"

/// \file
///

/// \brief context for a single FTP backed file
///
typedef struct tag_ftp_file
{
	open_attribute_t opened_for;	///< remember open attributes
	char local_path[MAX_PATH];		///< tempory file used to cache remote file content
	file_t *file;					///< file open on local file path
	credential_callback_t credential_callback;	///< callback to get user/pass
}
ftp_file_t;

/// \brief Close a ftp:// file
///
/// See ::file_close_t for details
///
static int file_ftp_close(file_t *file)
{
	ftp_file_t *remote_file;

	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (ftp_file_t*)file->priv;
	if (remote_file->file)
	{
		remote_file->file->file_close(file);
		remote_file->file = NULL;
		return 0;
	}
    return -1;
}

/// \brief Read a ftp:// file
///
/// See ::file_read_t for details
///
static int file_ftp_read(file_t *file, uint8_t *buffer, size_t count)
{
	ftp_file_t *remote_file;

	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (ftp_file_t*)file->priv;
	if (remote_file->file)
	{
		return remote_file->file->file_read(remote_file->file, buffer, count);
	}
    return -1;
}

/// \brief Write a ftp:// file
///
/// See ::file_write_t for details
///
static int file_ftp_write(file_t *file, uint8_t *buffer, size_t count)
{
	ftp_file_t *remote_file;
	int result;
	
	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (ftp_file_t*)file->priv;
	if (remote_file->file)
	{
		result = remote_file->file->file_write(remote_file->file, buffer, count);
		if (result)
		{
			return result;
		}
	}
	if (remote_file->opened_for == openForWrite || remote_file->opened_for == openForAppend)
	{
		char user[64];
		char pass[64];
		
		// get credentials for this url
		//
		user[0] = '\0';
		pass[0] = '\0';
		if (remote_file->credential_callback)
		{
			result = remote_file->credential_callback(file->url, user, sizeof(user), pass, sizeof(pass));
			if (result)
			{
				user[0] = '\0';
				pass[0] = '\0';
			}
		}
		// close/flush the backing local file
		//
		remote_file->file->file_close(remote_file->file);
		remote_file->file = NULL;
		
		// do an ftp put of the local file to the remote file
		//
	    result = bftp_put_file(
	                    file->url,
	                    remote_file->local_path,
	                    user,
	                    pass
	                    );
		if (result)
		{
			butil_log(1, "FTP Error putting %s\n", file->url);
		}
		// re-open the backing file
		//
		remote_file->file = file_create(remote_file->local_path, remote_file->opened_for);
		if (! remote_file->file)
		{
			butil_log(1, "Can't re-open file at path %s\n", remote_file->local_path);
			return -1;
		}	
	}
    return result;
}

/// \brief Seek in a ftp:// file
///
/// See ::file_seek_t for details
///
static int file_ftp_seek(file_t *file, uint64_t position)
{
	ftp_file_t *remote_file;

	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (ftp_file_t*)file->priv;
	if (remote_file->file)
	{
		return remote_file->file->file_seek(remote_file->file, position);
	}
    return -1;
}

int file_ftp_setup(file_t *file, open_attribute_t open_for, credential_callback_t credential_callback)
{
	ftp_file_t *remote_file;
	int result;
	
    file->file_close    = file_ftp_close;
    file->file_read     = file_ftp_read;
    file->file_write    = file_ftp_write;
    file->file_seek     = file_ftp_seek;
	
	// alloc a remote file context
	//
	remote_file = (ftp_file_t *)malloc(sizeof(ftp_file_t));
	if (! remote_file)
	{
		butil_log(1, "Can't alloc remote file context\n");
		return -1;
	}
	
	remote_file->file = NULL;
	remote_file->credential_callback = credential_callback;

	// get a temporary local file name
	//
	result = filesys_get_temp("ftp_local", remote_file->local_path, sizeof(remote_file->local_path));
	if (result)
	{
		butil_log(1, "Can't get local temp filename\n");
		free(remote_file);
		return -1;
	}
	
	if (open_for == openForRead || open_for == openForAppend)
	{
		char user[64];
		char pass[64];
		
		// get credentials for this url
		//
		user[0] = '\0';
		pass[0] = '\0';
		if (remote_file->credential_callback)
		{
			result = remote_file->credential_callback(file->url, user, sizeof(user), pass, sizeof(pass));
			if (result)
			{
				user[0] = '\0';
				pass[0] = '\0';
			}
		}
	    result = bftp_get_file(
	                    file->url,
	                    remote_file->local_path,
	                    user,
	                    pass
	                    );
		if (result)
		{
			butil_log(1, "FTP Error getting %s\n", file->url);
			free(remote_file);
			return result;
		}
	}

	file->priv = remote_file;
	
	// open the local backing-file
	//
	remote_file->file = file_create(remote_file->local_path, open_for);
	if (! remote_file->file)
	{
		butil_log(1, "Can't create file on path %s\n", remote_file->local_path);
		free(remote_file);
		return -1;
	}
    return result;
}

