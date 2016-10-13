noinst_PROGRAMS = client server

client_LDADD = \
	$(top_builddir)/src/waltham/libwaltham.la
client_CFLAGS = \
	-I$(top_builddir)/src/waltham/ \
	-I$(top_srcdir)/src/waltham/
client_SOURCES = \
	client-api-example.c \
	w-util.h

server_LDADD = \
	$(top_builddir)/src/waltham/libwaltham.la
server_CFLAGS = \
	-I$(top_builddir)/src/waltham/ \
	-I$(top_srcdir)/src/waltham/
server_SOURCES = \
	server-api-example.c \
	w-util.h \
	w-util.c
