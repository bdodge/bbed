#include <stdio.h>
#include <stdlib.h>

#include "bbuf.h"
#include "bfile.h"
#include "bfilesys.h"
#include "butil.h"

#define TEST_CHECK(condition, msg)		\
	if (!(condition)) do { butil_log(0, "FAIL: %s:%d - %s, %s\n", __FUNCTION__, __LINE__, #condition, msg); return -1; } while(0)

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

int httpbuffertest()
{
	file_t *file;
	buffer_t *buffer;
	int result;
	
	// get a known url
	//
	file = file_create("http://www.google.com/index.html", openForRead);
	TEST_CHECK(file != NULL, "Can't open googles index.html");
	
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("testing", file, NULL, 0); 
	TEST_CHECK(buffer != NULL, "Could not make buffer");

	// read the buffer
	//
	result = buffer_read(buffer);
	TEST_CHECK(result == 0, "Could not read buffer");
	
	buffer_destroy(buffer);	
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

int ftpbuffertest()
{
	file_t *file;
	buffer_t *buffer;
	int result;
	
	// get a known url
	//
	file = file_create_with_credentials("ftp://ftp.drivehq.com/test.jpg", openForRead, ftp_cred_callback);
	TEST_CHECK(file != NULL, "Can't open remote ftp file");
	
	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("testing", file, NULL, 0); 
	TEST_CHECK(buffer != NULL, "Could not make buffer");

	// read the buffer
	//
	result = buffer_read(buffer);
	TEST_CHECK(result == 0, "Could not read buffer");
	
	buffer_destroy(buffer);	
	file_destroy(file);
}

int buffertest()
{
	buffer_t *buffer;
	file_t *file;
	char filename[MAX_PATH];
	char tmpoutfilename[MAX_PATH];
	file_t *outfile;
	char data[128];
	uint8_t *linedata;
	char *linetext;
	size_t linelen;
	int cnt;
	int result;
	
	// create a temporay file
	//
	result = create_temp_file(&file, filename, sizeof(filename));
	TEST_CHECK(result == 0, "Can't make temp file");
	
	// put some data in it
	//
	strcpy(data, "hello\nworld\ndangle");
	cnt = file->file_write(file, data, strlen(data));
	TEST_CHECK(cnt == strlen(data), "Can't write File");
	
	// close it, to flush
	//
	file_destroy(file);
	
	// re-open for reading
	//
	file = file_create(filename, openForRead);
	TEST_CHECK(file != NULL, "Could not open file for read");

	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("testing", file, NULL, 0); 
	TEST_CHECK(buffer != NULL, "Could not make buffer");

	// read the buffer
	//
	result = buffer_read(buffer);
	TEST_CHECK(result == 0, "Could not read buffer");
	TEST_CHECK(buffer->line_count == 3, "Expected 3 lines in buffer");

	// access line 0
	//
	result = buffer_get_line_content(buffer, 0, &linedata, &linelen);
	TEST_CHECK(result == 0, "Can't edit line 0");
	TEST_CHECK(linelen == 6, "Expected 6 bytes in line 0");
	TEST_CHECK(!memcmp(linedata, "hello\n", linelen), "Expected \"hello\\n\" in line 0");

	// access line 1
	//
	result = buffer_get_line_content(buffer, 1, &linedata, &linelen);
	TEST_CHECK(result == 0, "Can't edit line 1");
	TEST_CHECK(linelen == 6, "Expected 6 bytes in line 1");
	TEST_CHECK(!memcmp(linedata, "world\n", linelen), "Expected \"world\\n\" in line 1");

	// access line 2
	//
	result = buffer_get_line_content(buffer, 2, &linedata, &linelen);
	TEST_CHECK(result == 0, "Can't edit line 2");
	TEST_CHECK(linelen == 6, "Expected 6 bytes in line 2");
	TEST_CHECK(!memcmp(linedata, "dangle", linelen), "Expected \"dangle\" in line 2");

	// access line 3
	//
	result = buffer_get_line_content(buffer, 3, &linedata, &linelen);
	TEST_CHECK(result != 0, "Could edit line 3 but no line there");

	// access line 1 again
	//
	result = buffer_get_line_content(buffer, 1, &linedata, &linelen);
	TEST_CHECK(result == 0, "Can't edit line 1");
	TEST_CHECK(linelen == 6, "Expected 6 bytes in line 1");
	TEST_CHECK(!memcmp(linedata, "world\n", linelen), "Expected \"world\\n\" in line 1");

	// Edit line 0
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	TEST_CHECK(result == 0, "Can't edit line 0");
	butil_log(2, "Line0=%s\n", linetext);
	TEST_CHECK(!strcmp(linetext, "hello\n"), "Expected null-terminated \"hello\\n");

	// create a new temp file
	//
	result = create_temp_file(&outfile, tmpoutfilename, sizeof(tmpoutfilename));
	TEST_CHECK(result == 0, "Can't create new temp file");
	
	// write the buffer to the outfile
	//
	result = buffer_write(buffer, outfile, textASCII);
	TEST_CHECK(result == 0, "Could not write out tempfile");

	file_destroy(outfile);
	
	// check contents of outfile [TODO]
	
	filesys_delete(tmpoutfilename);
	
	// destroy buffer
	//
	buffer_destroy(buffer);
	
	// and file
	//
	file_destroy(file);
	
	// delete file
	//
	filesys_delete(filename);
	
	return 0;
}

static int get_text_for_encoding(text_encoding_t encoding, bool nobom, char **text, size_t *txtlen)
{
	char *linetext;
	const char *encname;
	size_t linelen;
	
	switch (encoding)
	{
	case textBINARY:
	case textASCII:
	default:
		butil_log(0, "FAIL: %s:%d Bad encoding for encoded read test\n");
		*text = "";
		*txtlen = 0;
		return -1;
	case textUTF8:
		if (nobom) 
		{
			linetext = utf8nb_txt;
			linelen = utf8nb_txt_len;
			encname = "UTF-8";
		}
		else
		{
			linetext = utf8_txt;
			linelen = utf8_txt_len;
			encname = "UTF-8";
		}
		break;
	case textUCS2LE:
		linetext = ucs2le_txt;
		linelen = ucs2le_txt_len;
		encname = "UCS2-LE";
		break;
	case textUCS2BE:
		linetext = ucs2be_txt;
		linelen = ucs2be_txt_len;
		encname = "UCS2-BE";
		break;
	case textUCS4LE:
		linetext = ucs4le_txt;
		linelen = ucs4le_txt_len;
		encname = "UCS4-LE";
		break;
	case textUCS4BE:
		linetext = ucs4be_txt;
		linelen = ucs4be_txt_len;
		encname = "UCS4-BE";
		break;
	}
	*text = linetext;
	*txtlen = linelen;
	return 0;
}

int test_unicode_read(text_encoding_t encoding, bool nobom)
{
	buffer_t *buffer;
	buffer_t *outbuffer;
	file_t *file;
	file_t *outfile;
	char filename[MAX_PATH];
	char tmpoutfilename[MAX_PATH];
	char data[128];
	uint8_t *linedata;
	char *linetext;
	size_t linelen;
	int cnt;
	int result;

	result = get_text_for_encoding(encoding, nobom, &linetext, &linelen);
	TEST_CHECK(result == 0, "No text for encoding");
	
	// create a temporay file
	//
	result = create_temp_file(&file, filename, sizeof(filename));
	TEST_CHECK(result == 0, "Can't make temp file");
	
	// put encoded data in it
	//
	cnt = file->file_write(file, linetext, linelen);
	TEST_CHECK(cnt == linelen, "Didn't write all of encoded text");
	file_destroy(file);

	// re-open file for reading
	file = file_create(filename, openForRead);
	TEST_CHECK(file != NULL, "Could not open file for read");

	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	TEST_CHECK(buffer != NULL, "Could not make buffer");

	// read the buffer
	//
	result = buffer_read(buffer);
	TEST_CHECK(result == 0, "Could not read buffer");
	TEST_CHECK(buffer->original_encoding == encoding, "Didn't sniff expected encoding");

	// Edit line 0
	//
	result = buffer_edit_line(buffer, 0, &linetext, &linelen);
	TEST_CHECK(result == 0, "Can't edit line 0");
	butil_log(2, "Line0=%s\n", linetext);

	// Edit line 1
	//
	if (encoding == textUTF8)
	{
		result = buffer_edit_line(buffer, 1, &linetext, &linelen);
		TEST_CHECK(result == 0, "Can't edit line 1");
		butil_log(2, "Line1=%s\n", linetext);
	}
	else
	{
		result = buffer_edit_line(buffer, 1, &linetext, &linelen);
		TEST_CHECK(result != 0, "Could edit line 1");
	}	
	file_destroy(file);
	filesys_delete(filename);
}

int test_unicode_write(text_encoding_t encoding)
{
	buffer_t *buffer;
	buffer_t *outbuffer;
	file_t *file;
	file_t *outfile;
	char filename[MAX_PATH];
	char tmpoutfilename[MAX_PATH];
	char data[128];
	uint8_t *linedata;
	char *linetext;
	size_t linelen;
	int cnt;
	int result;

	// get utf8 text
	result = get_text_for_encoding(textUTF8, false, &linetext, &linelen);
	TEST_CHECK(result == 0, "No text for encoding");
	
	// create a temporay file
	//
	result = create_temp_file(&file, filename, sizeof(filename));
	TEST_CHECK(result == 0, "Can't make temp file");
	
	// put encoded data in it
	//
	cnt = file->file_write(file, linetext, linelen);
	TEST_CHECK(cnt == linelen, "Didn't write all of encoded text");
	file_destroy(file);

	// re-open file for reading
	file = file_create(filename, openForRead);
	TEST_CHECK(file != NULL, "Could not open file for read");

	// create a buffer to hold it using default buffering
	//
	buffer = buffer_create("unicode", file, NULL, 0); 
	TEST_CHECK(buffer != NULL, "Could not make buffer");

	// read the buffer
	//
	result = buffer_read(buffer);
	TEST_CHECK(result == 0, "Could not read buffer");
	TEST_CHECK(buffer->original_encoding == textUTF8, "Didn't sniff UTF-8 encoding");

	// create a new temp outfile
	//
	result = create_temp_file(&outfile, tmpoutfilename, sizeof(tmpoutfilename));
	TEST_CHECK(result == 0, "Can't make out temp file");

	// write previous buffer, using encoding, to new out file
	//
	result = buffer_write(buffer, outfile, encoding);
	TEST_CHECK(result == 0, "Could not write out tempfile");

	// close it / flush
	//
	file_destroy(outfile);
	
	// now re-open the out file for read
	//
	outfile = file_create(tmpoutfilename, openForRead);
	TEST_CHECK(outfile != NULL, "Could not open out tempfile for read");

	// buffer it
	//
	outbuffer = buffer_create("ucodecout", outfile, NULL, 0);
	TEST_CHECK(outbuffer != NULL, "Couldn't make out buffer");

	// read file into buffer
	//
	result = buffer_read(outbuffer);
	TEST_CHECK(result == 0, "Couldn't read buffer");
	
	// check that file was in expected encoding
	//
	TEST_CHECK(outbuffer->original_encoding == encoding, "Didn't sniff expected encoding");

	// check the first bytes for proper endianness and that BOM is stripped
	//
	result = buffer_get_line_content(outbuffer, 0, &linedata, &linelen);
	TEST_CHECK(result == 0, "Can't get line content");

	switch (encoding)
	{
	case textBINARY:
	case textASCII:
	default:
		butil_log(0, "FAIL: %s:%d Bad encoding for encoded write test\n");
		return -1;		
	case textUCS2LE:
		if (linedata[0] != 'T' || linedata[1] != '\0' || linedata[2] != 'h' || linedata[3] != '\0')
		{
			butil_log(0, "FAIL: %s:%d Expected \"T,0,h,0\" \"got %c0x%02X%c0x%02X\"\n",
					__FUNCTION__, __LINE__,
					linedata[0], linedata[1], linedata[2], linedata[3]);
			return -1;
		}
		break;
	case textUCS2BE:
		if (linedata[0] != '\0' || linedata[1] != 'T' || linedata[2] != '\0' || linedata[3] != 'h')
		{
			butil_log(0, "FAIL: %s:%d Expected \"0,t,0,h\" \"got 0x%02X%c0x%02X%c\"\n",
					__FUNCTION__, __LINE__,
					linedata[0], linedata[1], linedata[2], linedata[3]);
			return -1;
		}
		break;
	case textUCS4LE:
		if (linedata[0] != 'T' || linedata[1] != '\0' || linedata[2] != '\0' || linedata[3] != '\0')
		{
			butil_log(0, "FAIL: %s:%d Expected \"0,t,0,h\" \"got %c0x%02X0x%02X0x%02X\"\n",
					__FUNCTION__, __LINE__,
					linedata[0], linedata[1], linedata[2], linedata[3]);
			return -1;
		}
		break;
	case textUCS4BE:
		if (linedata[0] != '\0' || linedata[1] != '\0' || linedata[2] != '\0' || linedata[3] != 'T')
		{
			butil_log(0, "FAIL:%s:%d  Expected \"0,0,0,T\" \"got 0x%02X0x%02X0x%02X%c\"\n",
					__FUNCTION__, __LINE__,
					linedata[0], linedata[1], linedata[2], linedata[3]);
			return -1;
		}
		break;
	}
	buffer_destroy(outbuffer);
	file_destroy(outfile);
	filesys_delete(tmpoutfilename);
	file_destroy(file);
	filesys_delete(filename);
}

int unicodetest()
{
	int result;

	result = test_unicode_read(textUTF8, false);
	TEST_CHECK(result == 0, "UTF-8 Read");

	result = test_unicode_read(textUTF8, true);
	TEST_CHECK(result == 0, "UTF-8 with no byte-order-mark");

	result = test_unicode_read(textUCS2LE, true);
	TEST_CHECK(result == 0, "UCS2-LE Read");
	
	result = test_unicode_read(textUCS2BE, true);
	TEST_CHECK(result == 0, "UCS2-BE Read");
	
	result = test_unicode_read(textUCS4LE, true);
	TEST_CHECK(result == 0, "UCS4-LE Read");
	
	result = test_unicode_read(textUCS4BE, true);
	TEST_CHECK(result == 0, "UCS4-BE Read");
	
	result = test_unicode_write(textUCS2LE);
	TEST_CHECK(result == 0, "UCS2LE Write");

	result = test_unicode_write(textUCS2BE);
	TEST_CHECK(result == 0, "UCS2BE Write");

	result = test_unicode_write(textUCS4LE);
	TEST_CHECK(result == 0, "UCS4LE Write");

	result = test_unicode_write(textUCS4BE);
	TEST_CHECK(result == 0, "UCS4BE Write");

	return 0;
}

int main(int argc, char **argv)
{
	
	butil_set_log_level(5);

	if (httpbuffertest())
	{
		return -1;
	}
	if (ftpbuffertest())
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

