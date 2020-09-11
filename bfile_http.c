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
#include "bfile_http.h"
#include "bhttp.h"
#include "butil.h"

/// \file
///

/// \brief context for a single HTTP backed file
///
typedef struct tag_http_file
{
	open_attribute_t opened_for;	///< remember open attributes
	char local_path[MAX_PATH];		///< tempory file used to cache remote file content
	file_t *file;					///< file open on local file path
	credential_callback_t credential_callback;	///< callback to get user/pass
}
http_file_t;

/// \brief Close a http:// file
///
/// See ::file_close_t for details
///
static int file_http_close(file_t *file)
{
	http_file_t *remote_file;

	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (http_file_t*)file->priv;
	if (remote_file->file)
	{
		remote_file->file->file_close(file);
		remote_file->file = NULL;
		return 0;
	}
    return -1;
}

/// \brief Read a http:// file
///
/// See ::file_read_t for details
///
static int file_http_read(file_t *file, uint8_t *buffer, size_t count)
{
	http_file_t *remote_file;

	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (http_file_t*)file->priv;
	if (remote_file->file)
	{
		return remote_file->file->file_read(remote_file->file, buffer, count);
	}
    return -1;
}

/// \brief Write a http:// file
///
/// See ::file_write_t for details
///
static int file_http_write(file_t *file, uint8_t *buffer, size_t count)
{
	http_file_t *remote_file;
	int result;
	
	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (http_file_t*)file->priv;
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
        http_client_t *client;
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
		
		// do an http put of the local file to the remote file
		//
		butil_log(3, "%s:%d %s %s\n", __FUNCTION__, __LINE__, http_method_name(httpPut), file->url);

        client = http_client_create(NULL, true);
        if (! client)
        {
           	butil_log(1, "No memory for http client");
            return -1;
        }
		
        client->keepalive = false;

        do
        {
            result = http_client_request(
										client,
										httpPut,
										file->url,
										httpTCP,
										false,
										remote_file->local_path,
										NULL
										);

            while (! result)
            {
                result = http_client_slice(client);
                if (result)
                {
                    butil_log(2, "HTTP Client Error\n");
                    break;
                }
                if (client->state == httpDone)
                {
                    butil_log(4, "HTTP Client Complete\n");
                    break;
                }
				result = http_wait_for_client_event(client, 0, 10000);
                if (result < 0)
                {
                    break;
                }
                result = 0;
            }
        }
        while (!result && client->response >= 300 && client->response < 400);

		if (result)
		{
			butil_log(1, "HTTP Error putting %s\n", file->url);
		}
        butil_log(3, "HTTP Ends\n");
        http_client_free(client);
		
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

/// \brief Seek in a http:// file
///
/// See ::file_seek_t for details
///
static int file_http_seek(file_t *file, uint64_t position)
{
	http_file_t *remote_file;

	if (!file || !file->priv)
	{
		return -1;
	}
	remote_file = (http_file_t*)file->priv;
	if (remote_file->file)
	{
		return remote_file->file->file_seek(remote_file->file, position);
	}
    return -1;
}

int file_http_setup(file_t *file, open_attribute_t open_for, credential_callback_t credential_callback)
{
	http_file_t *remote_file;
	int result;
	
    file->file_close    = file_http_close;
    file->file_read     = file_http_read;
    file->file_write    = file_http_write;
    file->file_seek     = file_http_seek;
	
	// alloc a remote file context
	//
	remote_file = (http_file_t *)malloc(sizeof(http_file_t));
	if (! remote_file)
	{
		butil_log(1, "Can't alloc remote file context\n");
		return -1;
	}
	
	remote_file->file = NULL;

	// get a temporary local file name
	//
	result = file_get_temp("http_local", remote_file->local_path, sizeof(remote_file->local_path));
	if (result)
	{
		butil_log(1, "Can't get local temp filename\n");
		free(remote_file);
		return -1;
	}
	
	if (open_for == openForRead || open_for == openForAppend)
	{
		// do an http get of the remote file into the temporary file
		//
        http_client_t *client;
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

		butil_log(3, "%s:%d %s %s\n", __FUNCTION__, __LINE__, http_method_name(httpGet), file->url);

        client = http_client_create(NULL, true);
        if (! client)
        {
           	butil_log(1, "No memory for http client");
			free(remote_file);
            return -1;
        }
		
        client->keepalive = false;

        do
        {
            result = http_client_request(
										client,
										httpGet,
										file->url,
										httpTCP,
										false,
										remote_file->local_path,
										NULL
										);

            while (! result)
            {
                result = http_client_slice(client);
                if (result)
                {
                    butil_log(2, "HTTP Client Error\n");
                    break;
                }
                if (client->state == httpDone)
                {
                    butil_log(4, "HTTP Client Complete\n");
                    break;
                }
				result = http_wait_for_client_event(client, 0, 10000);
                if (result < 0)
                {
                    break;
                }
                result = 0;
            }
        }
        while (!result && client->response >= 300 && client->response < 400);

		if (result)
		{
			butil_log(1, "HTTP Error getting %s\n", file->url);
		}
        butil_log(3, "HTTP Ends\n");
        http_client_free(client);		
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

