AM_CFLAGS = @GCC_CFLAGS@ \
	    -I$(top_srcdir)/include \
	    -I$(top_srcdir)/src \
	    @GLIB_CFLAGS@ \
	    -DG_LOG_DOMAIN=\"grayley\"

lib_LTLIBRARIES = libgrayley.la

common_src = grayley.h

common_src +=				\
	grayley.c			\
	$(NULL)

libgrayley_la_SOURCES = $(common_src)

nodist_libgrayley_la_SOURCES = \
	$(top_srcdir)/src/util/message.c \
	$(top_srcdir)/src/util/util_generated.c \
	$(top_srcdir)/src/util/log.c

libgrayley_la_LIBADD = @GLIB_LIBS@ @WAYLAND_LIBS@
libgrayley_la_LDFLAGS = -lrt -version-info 1:0:0 -no-undefined

libgrayley_ladir = $(includedir)/grayley/
libgrayley_includedir = $(includedir)/grayley/
libgrayley_la_HEADERS = \
	$(top_srcdir)/include/waltham-message.h \
	$(top_srcdir)/src/grayley/grayley.h \
	$(top_srcdir)/src/util/message.h \
	$(top_srcdir)/src/util/log.h \
	$(top_srcdir)/src/util/profiling.h \
	$(top_srcdir)/src/util/util.h
