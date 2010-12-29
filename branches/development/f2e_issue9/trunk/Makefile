#
#  Copyright (C) 2010 Alexey Bobkov
#
#  This file is part of Fb2toepub converter.
#
#  Fb2toepub converter is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Fb2toepub converter is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Fb2toepub converter.  If not, see <http://www.gnu.org/licenses/>.
#

#
# Fb2toepub converter makefile for Linux/Unix
# Dependencies:
# 1) flex
# 2) zlib
#
distdir = unix_dist
srcdir = fb2toepub
incdir = include
libdir = lib
objdir = objs

ifndef $(VERSION)
VERSION=Custom Linux Build
endif

HSRC := $(wildcard $(srcdir)/*.h)

#CSRC := $(wildcard $(srcdir)/*.cpp) $(wildcard $(srcdir)/*.c) $(srcdir)/scanner.cpp
CSRC := $(wildcard $(srcdir)/*.c) \
		$(wildcard $(srcdir)/minizip/*.c) \
		$(wildcard $(srcdir)/tiniconv/*.c) \
		base64.cpp \
		convinfo.cpp \
		convpass1.cpp \
		convpass2.cpp \
		fb2toepub.cpp \
		fb2toepubconv.cpp \
		scandir.cpp \
		scanner.cpp \
		scannermisc.cpp \
		stream.cpp \
		streamtini.cpp \
		streamutf8.cpp \
		streamzip.cpp \
		translit.cpp \
		uuidmisc.cpp \
		mangling.cpp \
		types.cpp

COBJ=$(addprefix $(objdir)/, $(addsuffix .o, $(basename $(notdir $(CSRC)))))

# PRS-300/PRS-505 FLAGS
#CFLAGS= -O2 -I$(incdir) -D FB2TOEPUB_VERSION="$(VERSION)" -D FB2TOEPUB_NO_STD_STRING_COMPARE=1 -D TINICONV_NO_ASIAN_ENCODINGS

# default flags
CFLAGS= -O2 -D FB2TOEPUB_VERSION="$(VERSION)" -I$(incdir)

all : makedirs $(distdir)/fb2toepub

makedirs :
	mkdir -p $(objdir)
	mkdir -p $(distdir)

$(distdir)/fb2toepub : $(COBJ)
	g++ -o $@ $(COBJ) -lz
	strip $@

$(srcdir)/scanner.cpp : $(srcdir)/scanner.l
	flex -o$@ $<

$(objdir)/%.o : $(srcdir)/%.cpp $(HSRC)
	g++ $(CFLAGS) -o $@ -c $<

$(objdir)/%.o : $(srcdir)/%.c $(HSRC)
	gcc $(CFLAGS) -o $@ -c $<

$(objdir)/%.o : $(srcdir)/tiniconv/%.c $(HSRC)
	gcc $(CFLAGS) -o $@ -c $<

$(objdir)/%.o : $(srcdir)/minizip/%.c $(HSRC)
	gcc $(CFLAGS) -o $@ -c $<

 # The clean target
clean :
	rm -f $(objdir)/*.o
	rm -f $(distdir)/fb2toepub
