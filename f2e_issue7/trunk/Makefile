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

#CFLAGS= -O2 -I$(incdir) -D FB2TOEPUB_NO_STD_STRING_COMPARE=1
CFLAGS= -O2 -I$(incdir)

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
