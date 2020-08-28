#include <stdio.h>
#include <stdlib.h>

#include "bbuf.h"
#include "butil.h"

// unicode file data including byte-order-marks
//
unsigned char ucs2le_txt[] = {
  0xff, 0xfe, 0x54, 0x00, 0x68, 0x00, 0x69, 0x00, 0x73, 0x00, 0x20, 0x00,
  0x69, 0x00, 0x73, 0x00, 0x20, 0x00, 0x55, 0x00, 0x43, 0x00, 0x53, 0x00,
  0x32, 0x00, 0x4c, 0x00, 0x45, 0x00, 0x0a, 0x00, 0x0a, 0x00
};
unsigned int ucs2le_txt_len = 34;

unsigned char ucs2be_txt[] = {
  0xfe, 0xff, 0x00, 0x54, 0x00, 0x68, 0x00, 0x69, 0x00, 0x73, 0x00, 0x20,
  0x00, 0x69, 0x00, 0x73, 0x00, 0x20, 0x00, 0x55, 0x00, 0x43, 0x00, 0x53,
  0x00, 0x32, 0x00, 0x42, 0x00, 0x45, 0x00, 0x0a, 0x00, 0x0a
};
unsigned int ucs2be_txt_len = 34;

unsigned char ucs4le_txt[] = {
  0xff, 0xfe, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00,
  0x69, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x69, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x55, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00,
  0x34, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00
};
unsigned int ucs4le_txt_len = 68;

unsigned char ucs4be_txt[] = {
  0x00, 0x00, 0xfe, 0xff, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x68,
  0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x20,
  0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x20,
  0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x53,
  0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x45,
  0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0a
};
unsigned int ucs4be_txt_len = 68;

unsigned char utf8_txt[] = {
  0xef, 0xbb, 0xbf, 0x54, 0x68, 0x65, 0x20, 0x55, 0x6e, 0x69, 0x63, 0x6f,
  0x64, 0x65, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x20, 0x70, 0x6f, 0x69, 0x6e,
  0x74, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x22, 0xe2, 0x82, 0xac, 0x22, 0x20,
  0x69, 0x73, 0x20, 0x55, 0x2b, 0x32, 0x30, 0x41, 0x43, 0x2e, 0x0a, 'L',
  'i',  'n',  'e',  ' ',  '2',  0x0a
};
unsigned int utf8_txt_len = 54;

unsigned char utf8nb_txt[] = {
  0x54, 0x68, 0x65, 0x20, 0x55, 0x6e, 0x69, 0x63, 0x6f,
  0x64, 0x65, 0x20, 0x63, 0x6f, 0x64, 0x65, 0x20, 0x70, 0x6f, 0x69, 0x6e,
  0x74, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x22, 0xe2, 0x82, 0xac, 0x22, 0x20,
  0x69, 0x73, 0x20, 0x55, 0x2b, 0x32, 0x30, 0x41, 0x43, 0x2e, 0x0a, 'L',
  'i',  'n',  'e',  ' ',  '2',  0x0a
};
unsigned int utf8nb_txt_len = 51;

int filetest()
{
	file_t *f1;
	char tmpfile[MAX_PATH];
	char buffer[128];
	size_t fsize;
	time_t fmodtime;
	int result;
	int cnt;
	int rcnt;
	
	// create a temporay file
	//
	result = file_get_temp("bbuftemp", tmpfile, sizeof(tmpfile));
	if (result)
	{
		butil_log(0, "FAIL: Could not get temp file\n");
		return -1;
	}
	// open it for append
	//
	f1 = file_create(tmpfile, openForAppend);
	if (!f1)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp file %s\n", tmpfile);
	
	// put some data in it
	//
	strcpy(buffer, "hello\nworld\n");
	cnt = f1->file_write(f1, buffer, strlen(buffer));
	if (cnt != strlen(buffer))
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, strlen(buffer));
		file_destroy(f1);
		return -1;
	}
	// close it, to flush
	//
	file_destroy(f1);
	
	// get its info
	//
	result = file_info(tmpfile, &fsize, &fmodtime);
	if (result)
	{
		butil_log(0, "FAIL: Cant get tmpfile info\n");
		return -1;
	}
	butil_log(2, "tmpfile is %u bytes mod at %u\n", fsize, fmodtime);
	
	if (fsize != cnt)
	{
		butil_log(0, "FAIL: Expected tmpfile to be %d bytes, not %d\n", cnt, fsize);
		return -1;
	}
	// move the temp file to this directory
	//
	result = file_move(tmpfile, "file://./testfile.txt");
	if (result)
	{
		butil_log(0, "FAIL: Cant move tmpfile to testfile.txt\n");
		return -1;
	}
	// make sure it moved not copied
	//
	result = file_info(tmpfile, &fsize, &fmodtime);
	if (!result)
	{
		butil_log(0, "FAIL: tmpfile still has info\n");
		return -1;
	}
	// delete it now
	//
	result = file_delete("file://./testfile.txt");
	if (result)
	{
		butil_log(0, "FAIL: Cant delete testfile.txt\n");
		return -1;
	}		
	// open a non existing file for read
	//
	f1 = file_create("file://testfile.txt", openExistingForRead);
	if (f1)
	{
		butil_log(0, "FAIL: Could open testfile.txt\n");
		file_destroy(f1);
		return -1;
	}
	// now create the file
	//
	f1 = file_create("file://testfile.txt", openForWrite);
	if (!f1)
	{
		butil_log(0, "FAIL: Could not open testfile.txt for write\n");
		return -1;
	}
	// write some content
	//
	strcpy(buffer, "hello\nworld\n");
	cnt = f1->file_write(f1, buffer, strlen(buffer));
	if (cnt != strlen(buffer))
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, strlen(buffer));
		file_destroy(f1);
		return -1;
	}
	file_destroy(f1);
	
	// now open existing file for read
	//
	f1 = file_create("file://testfile.txt", openExistingForRead);
	if (!f1)
	{
		butil_log(0, "FAIL: Could not open testfile.txt for read\n");
		return -1;
	}
	// read the content
	//
	rcnt = f1->file_read(f1, buffer, sizeof(buffer));
	if (rcnt != cnt)
	{
		butil_log(0, "FAIL: Read only %d of %d\n", rcnt, cnt);
		file_destroy(f1);
		return -1;
	}
	// seek to offset 3
	//
	result = f1->file_seek(f1, 3);
	if (result)
	{
		butil_log(0, "FAIL: Seek failed\n");
		file_destroy(f1);
		return -1;
	}
	// read 2 bytes
	//
	rcnt = f1->file_read(f1, buffer, 2);
	if (rcnt != 2)
	{
		butil_log(0, "FAIL: Read only %d of %d\n", rcnt, 2);
		file_destroy(f1);
		return -1;
	}
	// make sure we got the right bytes
	//
	if (buffer[0] != 'l' && buffer[1] != 'o')
	{
		butil_log(0, "FAIL: Expected \"lo\" at offset 3, got %c%c\n", buffer[0], buffer[1]);
		file_destroy(f1);
		return -1;		
	}
	file_destroy(f1);	
	
	// cleanup
	//
	result = file_delete("testfile.txt");
	if (result)
	{
		butil_log(0, "FAIL: Can't delete file\n");
		return -1;
	}
	return 0;
}

int buffertest()
{
	buffer_t *buffer;
	file_t *file;
	char tmpfile[MAX_PATH];
	char tmpoutfile[MAX_PATH];
	file_t *outfile;
	char data[128];
	uint8_t *linedata;
	char *linetext;
	size_t linelen;
	int cnt;
	int result;
	
	// create a temporay file
	//
	result = file_get_temp("bbuftemp", tmpfile, sizeof(tmpfile));
	if (result)
	{
		butil_log(0, "FAIL: Could not get temp file\n");
		return -1;
	}
	// open it for append
	//
	file = file_create(tmpfile, openForAppend);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp file %s\n", tmpfile);
	
	// put some data in it
	//
	strcpy(data, "hello\nworld\ndangle");
	cnt = file->file_write(file, data, strlen(data));
	if (cnt != strlen(data))
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, strlen(data));
		file_destroy(file);
		return -1;
	}
	// close it
	//
	file_destroy(file);

	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("testing", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->line_count != 3)
	{
		butil_log(0, "FAIL: Expected 3 lines, got %d\n", buffer->line_count);
		return -1;
	}
	// access line 1
	//
	result = buffer_get_line_content(buffer, 0, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 0\n");
		return -1;
	}
	if (linelen != 6)
	{
		butil_log(0, "FAIL: line 0 Expected 6 bytes, got %d\n", linelen);
		return -1;
	}
	if (memcmp(linedata, "hello\n", linelen))
	{
		butil_log(0, "FAIL: line 0: expected \"hello\\n\" got \"%s\"\n", linedata);
		return -1;
	}	
	// access line 2
	//
	result = buffer_get_line_content(buffer, 1, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 2\n");
		return -1;
	}
	if (linelen != 6)
	{
		butil_log(0, "FAIL: line 1 Expected 6 bytes, got %d\n", linelen);
		return -1;
	}
	if (memcmp(linedata, "world\n", linelen))
	{
		butil_log(0, "FAIL: line 1: expected \"world\\n\" got \"%s\"\n", linedata);
		return -1;
	}
	// access line 3
	//
	result = buffer_get_line_content(buffer, 2, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 2\n");
		return -1;
	}
	if (linelen != 6)
	{
		butil_log(0, "FAIL: line 2 Expected 6 bytes, got %d\n", linelen);
		return -1;
	}
	if (memcmp(linedata, "dangle", linelen))
	{
		butil_log(0, "FAIL: line 1: expected \"dangle\" got \"%s\"\n", linedata);
		return -1;
	}
	// access line 4
	//
	result = buffer_get_line_content(buffer, 3, &linedata, &linelen);
	if (!result)
	{
		butil_log(0, "FAIL: Could edit line 4\n");
		return -1;
	}
	// access line 2 again
	//
	result = buffer_get_line_content(buffer, 1, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 2\n");
		return -1;
	}
	if (linelen != 6)
	{
		butil_log(0, "FAIL: line 1 Expected 6 bytes, got %d\n", linelen);
		return -1;
	}
	if (memcmp(linedata, "world\n", linelen))
	{
		butil_log(0, "FAIL: line 1: expected \"world\\n\" got \"%s\"\n", linedata);
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 2\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);
	if (strcmp(linetext, "hello\n"))
	{
		butil_log(0, "FAIL: Expected null-terminated \"hello\\n\", got \"%s\"\n", linetext);
		return -1;
	}
	// create a new temp file
	//
	result = file_get_temp("bbuftemp", tmpoutfile, sizeof(tmpoutfile));
	if (result)
	{
		butil_log(0, "FAIL: Could not get temp file\n");
		return -1;
	}
	// open it for append
	//
	outfile = file_create(tmpoutfile, openForAppend);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp out file %s\n", tmpoutfile);
	
	// write the buffer to the outfile
	//
	result = buffer_write(buffer, outfile, textASCII);
	if (result)
	{
		butil_log(0, "FAIL: Could not write out tempfile\n");
		return -1;
	}
	file_destroy(outfile);
	
	// check contents of outfile [TODO]
	
	file_delete(tmpoutfile);
	
	// destroy buffer
	//
	buffer_destroy(buffer);
	
	// and file
	//
	file_destroy(file);
	
	// delete file
	//
	file_delete(tmpfile);
	
	return 0;
}

int unicodetest()
{
	buffer_t *buffer;
	file_t *file;
	char tmpfile[MAX_PATH];
	char data[128];
	buffer_t *outbuffer;
	char tmpoutfile[MAX_PATH];
	file_t *outfile;
	uint8_t *linedata;
	char *linetext;
	size_t linelen;
	int cnt;
	int result;
	
	// create a temporay file
	//
	result = file_get_temp("bbuftemp", tmpfile, sizeof(tmpfile));
	if (result)
	{
		butil_log(0, "FAIL: Could not get temp file\n");
		return -1;
	}
	// ------------------ UTF-8
	//
	// open it for append
	//
	file = file_create(tmpfile, openForAppend);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UTF-8 data in it
	//
	cnt = file->file_write(file, utf8_txt, utf8_txt_len);
	if (cnt != utf8_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, utf8_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUTF8)
	{
		butil_log(0, "FAIL: Didn't sniff UTF-8\n");
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 1\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);

	// Edit line 2
	//
	result = buffer_edit_line(buffer, 1, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 2\n");
		return -1;
	}
	butil_log(2, "Line1=%s\n", linetext);
	
	file_destroy(file);

	// ------------------ UTF-8 No BOM
	//
	// open it for append
	//
	file = file_create(tmpfile, openForAppend);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UTF-8 data in it
	//
	cnt = file->file_write(file, utf8nb_txt, utf8nb_txt_len);
	if (cnt != utf8nb_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, utf8_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUTF8)
	{
		butil_log(0, "FAIL: Didn't sniff UTF-8\n");
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 1\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);

	// Edit line 2
	//
	result = buffer_edit_line(buffer, 1, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 2\n");
		return -1;
	}
	butil_log(2, "Line1=%s\n", linetext);
	
	file_destroy(file);

	// ------------------ UCS2-LE
	//
	// open it for write
	//
	file = file_create(tmpfile, openForWrite);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UCS2LE data in it
	//
	cnt = file->file_write(file, ucs2le_txt, ucs2le_txt_len);
	if (cnt != ucs2le_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, ucs2le_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUCS2LE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS2LE\n");
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 1\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);

	// ------------------ UCS2-BE
	//
	// open it for write
	//
	file = file_create(tmpfile, openForWrite);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UCS2BE data in it
	//
	cnt = file->file_write(file, ucs2be_txt, ucs2be_txt_len);
	if (cnt != ucs2be_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, ucs2be_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUCS2BE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS2BE\n");
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 1\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);

	// ------------------ UCS4-LE
	//
	// open it for write
	//
	file = file_create(tmpfile, openForWrite);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UCS4LE data in it
	//
	cnt = file->file_write(file, ucs4le_txt, ucs4le_txt_len);
	if (cnt != ucs4le_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, ucs4le_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUCS4LE)
	{
		butil_log(0, "FAIL: Didn't sniff UC4LE\n");
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 1\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);

	// ------------------ UCS4-BE
	//
	// open it for write
	//
	file = file_create(tmpfile, openForWrite);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UCS4BE data in it
	//
	cnt = file->file_write(file, ucs4be_txt, ucs4be_txt_len);
	if (cnt != ucs4be_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, ucs4be_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUCS4BE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS4BE\n");
		return -1;
	}
	// Edit line 1
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't edit line 1\n");
		return -1;
	}
	butil_log(2, "Line0=%s\n", linetext);

	// destroy buffer
	//
	buffer_destroy(buffer);
	
	// and file
	//
	file_destroy(file);
	
	// delete file
	//
	file_delete(tmpfile);
	
	//---------------------------------- writing ---------------------

	// create a temporay file
	//
	result = file_get_temp("bbuftemp", tmpfile, sizeof(tmpfile));
	if (result)
	{
		butil_log(0, "FAIL: Could not get temp file\n");
		return -1;
	}
	// open temp file for append
	//
	file = file_create(tmpfile, openForAppend);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open tempfile\n");
		return -1;
	}
	// put UTF-8 data in it
	//
	cnt = file->file_write(file, utf8nb_txt, utf8nb_txt_len);
	if (cnt != utf8nb_txt_len)
	{
		butil_log(0, "FAIL: Wrote only %d of %d\n", cnt, utf8_txt_len);
		return -1;
	}
	file_destroy(file);

	// re-open file for reading
	file = file_create(tmpfile, openExistingForRead);
	if (!file)
	{
		butil_log(0, "FAIL: Could not open file for read\n");
		return -1;
	}
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	if (!buffer)
	{
		butil_log(0, "FAIL: Couldn't make buffer\n");
		return -1;
	}
	// read the buffer
	//
	result = buffer_read(buffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (buffer->original_encoding != textUTF8)
	{
		butil_log(0, "FAIL: Didn't sniff UTF-8\n");
		return -1;
	}	
	// ----------------------- UCS2LE ---------------------------------
	//
	// create a new output temp file
	//
	result = file_get_temp("bbuftemp", tmpoutfile, sizeof(tmpoutfile));
	if (result)
	{
		butil_log(0, "FAIL: Could not get temp file\n");
		return -1;
	}
	// open it for append
	//
	outfile = file_create(tmpoutfile, openForAppend);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp out file %s for UCS2LE\n", tmpoutfile);
	
	// write the buffer to the outfile UCS2LE
	//
	result = buffer_write(buffer, outfile, textUCS2LE);
	if (result)
	{
		butil_log(0, "FAIL: Could not write out tempfile\n");
		return -1;
	}
	file_destroy(outfile);
	
	// open new out file for reading
	//
	outfile = file_create(tmpoutfile, openExistingForRead);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile for read\n");
		return -1;
	}
	// buffer it
	//
	outbuffer = buffer_create("ucodecout", outfile, NULL, 0);
	if (! outbuffer)
	{
		butil_log(0, "FAIL: Couldn't make out buffer\n");
		return -1;
	}
	// read it
	//
	result = buffer_read(outbuffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (outbuffer->original_encoding != textUCS2LE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS2LE\n");
		return -1;
	}
	// check the first 2 bytes for proper endianness and that BOM is stripped
	//
	result = buffer_get_line_content(outbuffer, 0, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't get line content\n");
		return -1;
	}
	if (linedata[0] != 'T' || linedata[1] != '\0' || linedata[2] != 'h' || linedata[3] != '\0')
	{
		butil_log(0, "FAIL: Expected \"T,0,h,0\" \"got %c0x%02X%c0x%02X\"\n",
				linedata[0], linedata[1], linedata[2], linedata[3]);
		return -1;
	}
	buffer_destroy(outbuffer);
	file_destroy(outfile);
	file_delete(tmpoutfile);
	
	// ----------------------- UCS2BE	
	//
	// open it for write
	//
	outfile = file_create(tmpoutfile, openForWrite);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp out file %s for UCS2BE\n", tmpoutfile);
	
	// write the buffer to the outfile UCS2BE
	//
	result = buffer_write(buffer, outfile, textUCS2BE);
	if (result)
	{
		butil_log(0, "FAIL: Could not write out tempfile\n");
		return -1;
	}
	file_destroy(outfile);
	
	// open new out file for reading
	//
	outfile = file_create(tmpoutfile, openExistingForRead);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile for read\n");
		return -1;
	}
	// buffer it
	//
	outbuffer = buffer_create("ucodecout", outfile, NULL, 0);
	if (! outbuffer)
	{
		butil_log(0, "FAIL: Couldn't make out buffer\n");
		return -1;
	}
	// read it
	//
	result = buffer_read(outbuffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (outbuffer->original_encoding != textUCS2BE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS2BE\n");
		return -1;
	}
	// check the first 2 bytes for proper endianness and that BOM is stripped
	//
	result = buffer_get_line_content(outbuffer, 0, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't get line content\n");
		return -1;
	}
	if (linedata[0] != '\0' || linedata[1] != 'T' || linedata[2] != '\0' || linedata[3] != 'h')
	{
		butil_log(0, "FAIL: Expected \"0,t,0,h\" \"got 0x%02X%c0x%02X%c\"\n",
				linedata[0], linedata[1], linedata[2], linedata[3]);
		return -1;
	}
	buffer_destroy(outbuffer);
	file_destroy(outfile);
	file_delete(tmpoutfile);
	
	// ----------------------- UCS4LE
	//
	// open it for write
	//
	outfile = file_create(tmpoutfile, openForWrite);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp out file %s for UCS4LE\n", tmpoutfile);
	
	// write the buffer to the outfile UCS2BE
	//
	result = buffer_write(buffer, outfile, textUCS4LE);
	if (result)
	{
		butil_log(0, "FAIL: Could not write out tempfile\n");
		return -1;
	}
	file_destroy(outfile);
	
	// open new out file for reading
	//
	outfile = file_create(tmpoutfile, openExistingForRead);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile for read\n");
		return -1;
	}
	// buffer it
	//
	outbuffer = buffer_create("ucodecout", outfile, NULL, 0);
	if (! outbuffer)
	{
		butil_log(0, "FAIL: Couldn't make out buffer\n");
		return -1;
	}
	// read it
	//
	result = buffer_read(outbuffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (outbuffer->original_encoding != textUCS4LE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS4LE\n");
		return -1;
	}
	// check the first 2 bytes for proper endianness and that BOM is stripped
	//
	result = buffer_get_line_content(outbuffer, 0, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't get line content\n");
		return -1;
	}
	if (linedata[0] != 'T' || linedata[1] != '\0' || linedata[2] != '\0' || linedata[3] != '\0')
	{
		butil_log(0, "FAIL: Expected \"0,t,0,h\" \"got %c0x%02X0x%02X0x%02X\"\n",
				linedata[0], linedata[1], linedata[2], linedata[3]);
		return -1;
	}
	buffer_destroy(outbuffer);
	file_destroy(outfile);
	file_delete(tmpoutfile);
			
	// ----------------------- UCS4BE	
	//
	// open it for write
	//
	outfile = file_create(tmpoutfile, openForWrite);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile\n");
		return -1;
	}
	butil_log(2, "Opened temp out file %s for UCS4BE\n", tmpoutfile);
	
	// write the buffer to the outfile UCS2BE
	//
	result = buffer_write(buffer, outfile, textUCS4BE);
	if (result)
	{
		butil_log(0, "FAIL: Could not write out tempfile\n");
		return -1;
	}
	file_destroy(outfile);
	
	// open new out file for reading
	//
	outfile = file_create(tmpoutfile, openExistingForRead);
	if (!outfile)
	{
		butil_log(0, "FAIL: Could not open out tempfile for read\n");
		return -1;
	}
	// buffer it
	//
	outbuffer = buffer_create("ucodecout", outfile, NULL, 0);
	if (! outbuffer)
	{
		butil_log(0, "FAIL: Couldn't make out buffer\n");
		return -1;
	}
	// read it
	//
	result = buffer_read(outbuffer);
	if (result)
	{
		butil_log(0, "FAIL: Couldn't read buffer\n");
		return -1;
	}
	if (outbuffer->original_encoding != textUCS4BE)
	{
		butil_log(0, "FAIL: Didn't sniff UCS4BE\n");
		return -1;
	}
	// check the first 2 bytes for proper endianness and that BOM is stripped
	//
	result = buffer_get_line_content(outbuffer, 0, &linedata, &linelen);
	if (result)
	{
		butil_log(0, "FAIL: Can't get line content\n");
		return -1;
	}
	if (linedata[0] != '\0' || linedata[1] != '\0' || linedata[2] != '\0' || linedata[3] != 'T')
	{
		butil_log(0, "FAIL: Expected \"0,0,0,T\" \"got 0x%02X0x%02X0x%02X%c\"\n",
				linedata[0], linedata[1], linedata[2], linedata[3]);
		return -1;
	}
	buffer_destroy(outbuffer);
	file_destroy(outfile);
	file_delete(tmpoutfile);
	
	// destroy buffer
	//
	buffer_destroy(buffer);
	
	// and file
	//
	file_destroy(file);
	
	// delete file
	//
	file_delete(tmpfile);
	
	return 0;
}

int main(int argc, char **argv)
{
	
	butil_set_log_level(5);

	if (filetest())
	{
		return -1;
	}
	if (buffertest())
	{
		return -1;
	}
	if (unicodetest())
	{
		return 1;
	}
	butil_log(0, "PASS\n");
	return 0;
}

