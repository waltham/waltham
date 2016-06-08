BUILT_SOURCES = \
	client-serialice.c \
	client-deserialice.c \
	server-serialice.c \
	server-deserialice.c \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = serial-preamble.txt deserial-preamble.txt

AM_CFLAGS = \
	@GCC_CFLAGS@ @GLIB_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gael \
	-I$(top_srcdir)/src/grayley \
	-I$(top_srcdir)/src \
	$(NULL)

lib_LTLIBRARIES = libwalthamclient.la libwalthamserver.la

libwalthamclient_la_LDFLAGS = -version-info 0:0:0 -no-undefined
libwalthamclient_la_LIBADD = \
	$(top_builddir)/src/gael/libgael.la

libwalthamserver_la_LDFLAGS = -version-info 0:0:0 -no-undefined
libwalthamserver_la_LIBADD = \
	$(top_builddir)/src/gael/libgael.la

common_src = \
	waltham-object.c \
	waltham-object.h \
	$(NULL)

tools = \
	$(top_srcdir)/tools/gen.py			\
	$(top_srcdir)/tools/customcode.py		\
	$(top_srcdir)/tools/varsize.py

core_interface = \
	$(top_srcdir)/data/command.xml

client-serialice.c: $(tools) $(core_interface) $(extensions) $(srcdir)/serial-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/serial-preamble.txt \
		-i $(core_interface) \
		-o $@ \
		-m client \
		-t gael

client-deserialice.c: $(core_interface) $(tools) $(srcdir)/deserial-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/deserial-preamble.txt \
		-i $(top_srcdir)/data/command.xml \
		-o $@ \
		-m client \
		-t grayley

libwalthamclient_la_SOURCES = \
	$(common_src)

nodist_libwalthamclient_la_SOURCES = \
	client-serialice.c \
	client-deserialice.c

server-serialice.c: $(tools) $(core_interface) $(extensions) $(srcdir)/serial-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/serial-preamble.txt \
		-i $(core_interface) \
		-o $@ \
		-m server \
		-t gael

server-deserialice.c: $(core_interface) $(tools) $(srcdir)/deserial-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/deserial-preamble.txt \
		-i $(top_srcdir)/data/command.xml \
		-o $@ \
		-m server \
		-t grayley

libwalthamserver_la_SOURCES = \
	$(common_src)

nodist_libwalthamserver_la_SOURCES = \
	server-serialice.c \
	server-deserialice.c
