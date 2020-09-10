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

SOURCES=$(SRCDIR)/bbuf.c $(SRCDIR)/bline.c $(SRCDIR)/bfile.c $(SRCDIR)/bundo.c
HEADERS=$(SOURCES:%.c=%.h)
OBJECTS=$(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

LIBS= $(IOLIB) $(UTILLIB) $(HTTPLIB)
LIBDIRS= $(IO_PATH) $(UTIL_PATH) $(HTTP_PATH)
LIBINCLS= $(LIBDIRS:%=-I%)
CFLAGS += $(LIBINCLS) -g 

PROGSOURCES=$(SRCDIR)/bbuftest.c
PROGOBJECTS=$(OBJDIR)/bbuftest.o

bbuftest: $(PROGOBJECTS) $(OBJDIR)/bbuf.a $(LIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(SYSLIBS)

library: $(OBJDIR)/bbuf.a

$(OBJDIR)/bbuf.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

clean:
	rm $(OBJECTS) $(OBJDIR)/bbuf.a

$(OBJDIR)/bbuf.o: $(SRCDIR)/bbuf.c $(HEADERS)
$(OBJDIR)/bline.o: $(SRCDIR)/bline.c $(HEADERS)
$(OBJDIR)/bfile.o: $(SRCDIR)/bfile.c $(HEADERS)
$(OBJDIR)/bbuftest.o: $(SRCDIR)/bbuftest.c $(HEADERS)

