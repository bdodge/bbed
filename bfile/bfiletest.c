#include <stdio.h>
#include <stdlib.h>

#include "bfile.h"
#include "bfilesys.h"
#include "butil.h"

#define TEST_CHECK(condition, msg)		\
	if (!(condition)) do { butil_log(0, "FAIL: %s:%d - %s, %s\n", __FUNCTION__, __LINE__, #condition, msg); return -1; } while(0)

static int create_temp_file(file_t **pfile, char *name, size_t nname)
{
	file_t *file;
	char tmpfilename[MAX_PATH];
	int result;
	
	// get a temp file name
	//
	result = filesys_get_temp("bbuftemp", tmpfilename, sizeof(tmpfilename));
	if (result)
	{
		butil_log(0, "FAIL: %s: Could not get temp file\n", __FUNCTION__);
		return result;
	}
	// open it for append
	//
	file = file_create(tmpfilename, openForAppend);
	if (!file)
	{
		butil_log(0, "FAIL: %s: Could not open tempfile %s\n", __FUNCTION__, tmpfilename);
		return -1;
	}
	butil_log(2, "Opened temp file %s\n", tmpfilename);
	if (name && nname)
	{
		strncpy(name, tmpfilename, nname - 1);
		tmpfilename[nname - 1] = '\0';
	}
	*pfile = file;
	return 0;
}

int filetest()
{
	file_t *file;
	char filename[MAX_PATH];
	char buffer[128];
	size_t fsize;
	time_t fmodtime;
	int result;
	int cnt;
	int rcnt;

	result = create_temp_file(&file, filename, sizeof(filename));
	TEST_CHECK(result == 0, "Can't create temp file");
	
	// put some data in it
	//
	strcpy(buffer, "hello\nworld\n");
	cnt = file->file_write(file, buffer, strlen(buffer));
	TEST_CHECK(cnt == strlen(buffer), "Can't write File");
	
	// close it, to flush
	//
	file_destroy(file);
	
	// get its info
	//
	result = filesys_info(filename, &fsize, &fmodtime);
	TEST_CHECK(result == 0, "Can't Get filesys_info");

	butil_log(2, "tmpfile is %u bytes mod at %u\n", fsize, fmodtime);
	
	TEST_CHECK(fsize == strlen(buffer), "File is not size of buffer written");

	// move the temp file to this directory
	//
	result = filesys_move(filename, "file://./testfile.txt");
	if (result)
	{
		butil_log(0, "FAIL: Cant move tmpfile to testfile.txt\n");
		return -1;
	}
	// make sure it moved not copied
	//
	result = filesys_info(filename, &fsize, &fmodtime);
	TEST_CHECK(result != 0, "temp file still exists (wasn't moved)");

	// delete it now
	//
	result = filesys_delete("file://./testfile.txt");
	TEST_CHECK(result == 0, "Can't delete testfile");

	// open a non existing file for read
	//
	file = file_create("file://testfile.txt", openForRead);
	TEST_CHECK(file == NULL, "Could open non-existing file for read");

	// now create the file
	//
	file = file_create("file://testfile.txt", openForWrite);
	TEST_CHECK(file != NULL, "Count not open testfile.txt for write");

	// write some content
	//
	strcpy(buffer, "hello\nworld\n");
	cnt = file->file_write(file, buffer, strlen(buffer));
	TEST_CHECK(cnt == strlen(buffer), "Didn't write whole of buffer");

	file_destroy(file);
	
	// now open existing file for read
	//
	file = file_create("file://testfile.txt", openForRead);
	TEST_CHECK(file != NULL, "Could not open testfile.txt for read");

	// read the content
	//
	rcnt = file->file_read(file, buffer, sizeof(buffer));
	TEST_CHECK(cnt == strlen(buffer), "Didn't read whole of buffer");

	// seek to offset 3
	//
	result = file->file_seek(file, 3);
	TEST_CHECK(result == 0, "Seek Failed");

	// read 2 bytes
	//
	rcnt = file->file_read(file, buffer, 2);
	TEST_CHECK(rcnt == 2, "Didn't read exactly 2 bytes");

	// make sure we got the right bytes
	//
	TEST_CHECK(buffer[0] == 'l' && buffer[1] == 'o', "Didn't read \"lo\" at offset 3");

	file_destroy(file);
	
	// cleanup
	//
	result = filesys_delete("testfile.txt");
	TEST_CHECK(result == 0, "Can't delete file");

	return 0;
}

int httpfiletest()
{
	file_t *file;
	int result;
	
	// get a known url
	//
	file = file_create("http://www.google.com/index.html", openForRead);
	TEST_CHECK(file != NULL, "Can't open googles index.html");

	file_destroy(file);
}

static int ftp_cred_callback(const char *url, char *username, size_t nusername, char *password, size_t npassword)
{
	strncpy(username, "bnet_test_account", nusername - 1);
	username[nusername - 1] = '\0';
	strncpy(password, "jabberwocky", npassword - 1);
	username[npassword - 1] = '\0';
	return 0;
}

int ftpfiletest()
{
	file_t *file;
	int result;
	
	// get a known url
	//
	file = file_create_with_credentials("ftp://ftp.drivehq.com/test.jpg", openForRead, ftp_cred_callback);
	TEST_CHECK(file != NULL, "Can't open remote ftp file");

	file_destroy(file);
}

int main(int argc, char **argv)
{
	
	butil_set_log_level(5);

	if (filetest())
	{
		return -1;
	}
	if (httpfiletest())
	{
		return -1;
	}
	if (ftpfiletest())
	{
		return -1;
	}
	butil_log(0, "PASS\n");
	return 0;
}

