srcdir = .

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

CC = gcc
INSTALL = /usr/bin/install -c

DEFS = -DPACKAGE_NAME=\"tweeta-cli\" -DPACKAGE_TARNAME=\"tweeta-cli\" -DPACKAGE_VERSION=\"0.1.0\" -DPACKAGE_STRING=\"tweeta-cli\ 0.1.0\" -DPACKAGE_BUGREPORT=\"https://github.com/tweetapus/tweeta-cli/issues\" -DPACKAGE_URL=\"\" -DHAVE_STDIO_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_STRINGS_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_UNISTD_H=1 -DHAVE_WCHAR_H=1 -DSTDC_HEADERS=1 -D_ALL_SOURCE=1 -D_DARWIN_C_SOURCE=1 -D_GNU_SOURCE=1 -D_HPUX_ALT_XOPEN_SOCKET_API=1 -D_NETBSD_SOURCE=1 -D_OPENBSD_SOURCE=1 -D_POSIX_PTHREAD_SEMANTICS=1 -D__STDC_WANT_IEC_60559_ATTRIBS_EXT__=1 -D__STDC_WANT_IEC_60559_BFP_EXT__=1 -D__STDC_WANT_IEC_60559_DFP_EXT__=1 -D__STDC_WANT_IEC_60559_EXT__=1 -D__STDC_WANT_IEC_60559_FUNCS_EXT__=1 -D__STDC_WANT_IEC_60559_TYPES_EXT__=1 -D__STDC_WANT_LIB_EXT2__=1 -D__STDC_WANT_MATH_SPEC_FUNCS__=1 -D_TANDEM_SOURCE=1 -D__EXTENSIONS__=1
CPPFLAGS = 
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = 
LIBS = 
LIBCURL_CFLAGS = -I/usr/include/p11-kit-1
LIBCURL_LIBS = -lcurl

EXEEXT = 
OBJEXT = o

PROGRAM = tweeta$(EXEEXT)
OBJ = \
	main.$(OBJEXT) \
	config.$(OBJEXT) \
	util.$(OBJEXT) \
	http.$(OBJEXT) \
	routes.$(OBJEXT) \
	commands.$(OBJEXT) \
	jsonfmt.$(OBJEXT) \
	cJSON.$(OBJEXT)

COMPILE = $(CC) $(DEFS) $(CPPFLAGS) $(LIBCURL_CFLAGS) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS)

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(LINK) -o $@ $(OBJ) $(LIBCURL_LIBS) $(LIBS)

main.$(OBJEXT): $(srcdir)/src/main.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/main.c

config.$(OBJEXT): $(srcdir)/src/config.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/config.c

util.$(OBJEXT): $(srcdir)/src/util.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/util.c

http.$(OBJEXT): $(srcdir)/src/http.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/http.c

routes.$(OBJEXT): $(srcdir)/src/routes.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/routes.c

commands.$(OBJEXT): $(srcdir)/src/commands.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/commands.c

jsonfmt.$(OBJEXT): $(srcdir)/src/jsonfmt.c $(srcdir)/src/tweeta.h $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/jsonfmt.c

cJSON.$(OBJEXT): $(srcdir)/src/cJSON.c $(srcdir)/src/cJSON.h
	$(COMPILE) -c -o $@ $(srcdir)/src/cJSON.c

install: $(PROGRAM)
	$(INSTALL) -d "$(DESTDIR)$(bindir)"
	$(INSTALL) -m 0755 $(PROGRAM) "$(DESTDIR)$(bindir)/$(PROGRAM)"

clean:
	rm -f $(PROGRAM) $(OBJ)

distclean: clean
	rm -f Makefile config.cache config.log config.status
	rm -rf autom4te.cache

.PHONY: all install clean distclean
