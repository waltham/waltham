BUILT_SOURCES = \
	waltham-client.h \
	client-serialice.c \
	client-deserialice.c \
	waltham-server.h \
	server-serialice.c \
	server-deserialice.c \
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = header-preamble.txt serial-preamble.txt deserial-preamble.txt

AM_CFLAGS = \
	@GCC_CFLAGS@ @GLIB_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/marshaller \
	-I$(top_srcdir)/src \
	$(NULL)

lib_LTLIBRARIES = libwaltham.la

libwaltham_la_LDFLAGS = -version-info 0:0:0 -no-undefined
libwaltham_la_LIBADD = \
	$(top_builddir)/src/util/libwalthamutil.la

common_src = \
	marshaller.c \
	marshaller.h \
	marshaller_log.h \
	waltham-connection.c \
	waltham-connection.h \
	waltham-object.c \
	waltham-object.h \
	waltham-private.h \
	waltham-util.c \
	waltham-util.h \
	$(NULL)

tools = \
	$(top_srcdir)/tools/gen.py

core_interface = \
	$(top_srcdir)/data/command.xml

waltham-client.h: $(tools) $(core_interface) $(extensions) $(srcdir)/header-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/header-preamble.txt \
		-i $(core_interface) \
		-o $@ \
		-m client \
		-t header

client-serialice.c: $(tools) $(core_interface) $(extensions) $(srcdir)/serial-preamble.txt waltham-client.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/serial-preamble.txt \
		-i $(core_interface) \
		-o $@ \
		-m client \
		-t marshaller

client-deserialice.c: $(core_interface) $(tools) $(srcdir)/deserial-preamble.txt waltham-client.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/deserial-preamble.txt \
		-i $(top_srcdir)/data/command.xml \
		-o $@ \
		-m client \
		-t demarshaller

waltham-server.h: $(tools) $(core_interface) $(extensions) $(srcdir)/header-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/header-preamble.txt \
		-i $(core_interface) \
		-o $@ \
		-m server \
		-t header

server-serialice.c: $(tools) $(core_interface) $(extensions) $(srcdir)/serial-preamble.txt waltham-server.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/serial-preamble.txt \
		-i $(core_interface) \
		-o $@ \
		-m server \
		-t marshaller

server-deserialice.c: $(core_interface) $(tools) $(srcdir)/deserial-preamble.txt waltham-server.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/deserial-preamble.txt \
		-i $(top_srcdir)/data/command.xml \
		-o $@ \
		-m server \
		-t demarshaller

libwaltham_la_SOURCES = \
	$(common_src)

nodist_libwaltham_la_SOURCES = \
	client-serialice.c \
	client-deserialice.c \
	server-serialice.c \
	server-deserialice.c

include_HEADERS = \
	waltham-connection.h \
	waltham-object.h \
	waltham-util.h \
	$(NULL)

nodist_include_HEADERS = \
	waltham-client.h \
	waltham-server.h
