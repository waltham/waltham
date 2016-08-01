AM_CFLAGS = @GCC_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/include

BUILT_SOURCES = util_generated.c
CLEANFILES = $(BUILT_SOURCES)

tools = \
	$(top_srcdir)/tools/gen.py			\
	$(top_srcdir)/tools/varsize.py

extensions = \
	$(top_srcdir)/data/command.xml

util_generated.c: $(extensions) $(tools)
	$(top_srcdir)/tools/gen.py \
		-i $(top_srcdir)/data/command.xml \
		-o ./util_generated.c \
		-t util

noinst_LTLIBRARIES = libwalthamutil.la
libwalthamutil_la_SOURCES = 	\
	log.c			\
	log.h			\
	message.c		\
	message.h		\
	profiling.h

nodist_libwalthamutil_la_SOURCES = \
	util_generated.c

libwalthamutil_la_LIBADD = @GLIB_LIBS@
libwalthamutil_la_LDFLAGS = -no-undefined
