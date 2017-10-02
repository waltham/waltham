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

AM_CFLAGS = @GCC_CFLAGS@

lib_LTLIBRARIES = libwaltham.la

libwaltham_la_LDFLAGS = -version-info @VERSION_INFO@ -no-undefined

tools = \
	$(top_srcdir)/tools/gen.py

core_interface = \
	$(top_srcdir)/data/private.xml \
	$(top_srcdir)/data/public.xml \
	$(top_srcdir)/data/command.xml

core_interface_include := $(addprefix -i ,$(core_interface))

waltham-client.h: $(tools) $(core_interface) $(extensions) $(srcdir)/header-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/header-preamble.txt \
		$(core_interface_include) \
		-o $@ \
		-m client \
		-t header

client-serialice.c: $(tools) $(core_interface) $(extensions) $(srcdir)/serial-preamble.txt waltham-client.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/serial-preamble.txt \
		$(core_interface_include) \
		-o $@ \
		-m client \
		-t marshaller

client-deserialice.c: $(core_interface) $(tools) $(srcdir)/deserial-preamble.txt waltham-client.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/deserial-preamble.txt \
		$(core_interface_include) \
		-o $@ \
		-m client \
		-t demarshaller

waltham-server.h: $(tools) $(core_interface) $(extensions) $(srcdir)/header-preamble.txt
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/header-preamble.txt \
		$(core_interface_include) \
		-o $@ \
		-m server \
		-t header

server-serialice.c: $(tools) $(core_interface) $(extensions) $(srcdir)/serial-preamble.txt waltham-server.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/serial-preamble.txt \
		$(core_interface_include) \
		-o $@ \
		-m server \
		-t marshaller

server-deserialice.c: $(core_interface) $(tools) $(srcdir)/deserial-preamble.txt waltham-server.h
	$(top_srcdir)/tools/gen.py \
		-p $(srcdir)/deserial-preamble.txt \
		$(core_interface_include) \
		-o $@ \
		-m server \
		-t demarshaller

libwaltham_la_SOURCES = \
	demarshaller.h \
	marshaller.c \
	marshaller.h \
	marshaller_log.h \
	message.c \
	message.h \
	waltham-connection.c \
	waltham-connection.h \
	waltham-object.c \
	waltham-object.h \
	waltham-private.h \
	waltham-util.c \
	waltham-util.h \
	$(NULL)

nodist_libwaltham_la_SOURCES = \
	client-serialice.c \
	client-deserialice.c \
	server-serialice.c \
	server-deserialice.c

waltham_includedir = $(includedir)/waltham
waltham_include_HEADERS = \
	waltham-connection.h \
	waltham-object.h \
	waltham-util.h \
	$(NULL)

nodist_waltham_include_HEADERS = \
	waltham-client.h \
	waltham-server.h
