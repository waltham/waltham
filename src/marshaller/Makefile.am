AM_CFLAGS = @GCC_CFLAGS@ @GLIB_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/waltham \
	$(NULL)

noinst_LTLIBRARIES = libmarshaller.la
libmarshaller_la_SOURCES = marshaller.c marshaller.h

EXTRA_DIST = \
	preamble.txt \
	marshaller_log.h \
	$(NULL)
