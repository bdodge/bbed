#
# Copyright 2020 Brian Dodge
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

SRCROOT=../../bnet
include $(SRCROOT)/common/makecommon.mk

SOURCES=$(SRCDIR)/bfile.c $(SRCDIR)/bfilesys.c \
	$(SRCDIR)/bfile_file.c $(SRCDIR)/bfile_http.c $(SRCDIR)/bfile_ftp.c
HEADERS=$(SOURCES:%.c=%.h)
OBJECTS=$(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

LIBS=  $(HTTPLIB) $(FTPLIB) $(IOLIB) $(UTILLIB)
LIBDIRS= $(IO_PATH) $(UTIL_PATH) $(HTTP_PATH)  $(FTP_PATH)
LIBINCLS= $(LIBDIRS:%=-I%)
CFLAGS += $(LIBINCLS)
EXTRA_DEFINES += "HTTP_SUPPORT_WEBSOCKET=0 HTTP_SUPPORT_WEBDAV=0"

PROGSOURCES=$(SRCDIR)/bfiletest.c
PROGOBJECTS=$(OBJDIR)/bfiletest.o

bfiletest: $(PROGOBJECTS) $(OBJDIR)/bfile.a $(LIBS) $(TLSDEPS) $(ZLIBDEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(SYSLIBS)

library: $(OBJDIR)/bfile.a

$(OBJDIR)/bfile.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm -f $(OBJECTS) $(OBJDIR)/bfile.a $(PROGOBJECTS) bfiletest

$(OBJDIR)/bfile.o: $(SRCDIR)/bfile.c $(HEADERS)
$(OBJDIR)/bfilesys.o: $(SRCDIR)/bfilesys.c $(HEADERS)
$(OBJDIR)/bfile_file.o: $(SRCDIR)/bfile_file.c $(HEADERS)
$(OBJDIR)/bfile_http.o: $(SRCDIR)/bfile_http.c $(HEADERS)
$(OBJDIR)/bfile_ftp.o: $(SRCDIR)/bfile_ftp.c $(HEADERS)

$(OBJDIR)/bfiletest.o: $(SRCDIR)/bfiletest.c $(HEADERS)

