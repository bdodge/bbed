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
#include <time.h>
#include "bfilesys.h"
#include "bfile.h"
#include "bfile_file.h"
#include "bfile_http.h"
#include "bfile_ftp.h"
#include "butil.h"

/// \file
///

int filesys_info(const char *url, size_t *size, time_t *mod_time)
{
    char path[MAX_PATH];
    struct stat fstat;
    butil_url_scheme_t scheme;
    int result;

    scheme = file_get_scheme(url, path, sizeof(path));
    
    switch (scheme)
    {
    case schemeFILE:
        result = stat(path, &fstat);
        if (result)
        {
            butil_log(2, "No such path: %s\n", path);
            return -1;
        }
        if (size)
        {
            *size = fstat.st_size;
        }
        if (mod_time)
        {
            *mod_time = fstat.st_mtime;
        }
        return 0;
        
    case schemeDAV:
        return 0;
    }
    butil_log(1, "Scheme %s not implemented for %s\n", butil_scheme_name(scheme), __FUNCTION__);
    return -1;
}

int filesys_delete(const char *url)
{
    char path[MAX_PATH];
    butil_url_scheme_t scheme;
    int result;

    scheme = file_get_scheme(url, path, sizeof(path));
    
    if (scheme == schemeFILE)
    {
        result = unlink(path);
        if (result)
        {
            butil_log(2, "Unlink of %s failed\n", path);
        }
        return result;
    }
    butil_log(1, "Scheme %s not implemented for %s\n", butil_scheme_name(scheme), __FUNCTION__);
    return -1;
}

int filesys_move(const char *source_url, const char *destination_url)
{
    char src_path[MAX_PATH];
    char dst_path[MAX_PATH];
    butil_url_scheme_t src_scheme;
    butil_url_scheme_t dst_scheme;
    int result;

    src_scheme = file_get_scheme(source_url, src_path, sizeof(src_path));
    dst_scheme = file_get_scheme(destination_url, dst_path, sizeof(dst_path));
    
    if (src_scheme == schemeFILE && dst_scheme == schemeFILE)
    {
        result = rename(src_path, dst_path);
        if (result)
        {
            butil_log(2, "Rename of %s to %s failed\n", src_path, dst_path);
        }
        return result;
    }
    butil_log(1, "Scheme %s->%s not implemented for %s\n",
           butil_scheme_name(src_scheme), butil_scheme_name(dst_scheme), __FUNCTION__);
    return -1;
}

int filesys_get_temp(const char *hint, char *url, const size_t size)
{
    int tmpfile;
    
    if (!url || !size)
    {
        return -1;
    }
    snprintf(url, size, "%s.XXXXXX", hint ? hint : "bfile");
    tmpfile = mkstemp(url);
    if (tmpfile < 0)
    {
        butil_log(1, "mkstmp failed in %s\n", __FUNCTION__);
        return -1;
    }
    close(tmpfile);
    return 0;
}

