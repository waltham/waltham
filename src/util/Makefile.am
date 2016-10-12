AM_CFLAGS = @GCC_CFLAGS@ \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/include

tools = \
	$(top_srcdir)/tools/gen.py

extensions = \
	$(top_srcdir)/data/command.xml

noinst_LTLIBRARIES = libwalthamutil.la
libwalthamutil_la_SOURCES = 	\
	message.c		\
	message.h

libwalthamutil_la_LIBADD = @GLIB_LIBS@
libwalthamutil_la_LDFLAGS = -no-undefined
