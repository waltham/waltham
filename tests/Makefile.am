noinst_PROGRAMS = client server

client_LDADD = \
	@GLIB_LIBS@ \
	$(top_builddir)/src/util/libwalthamutil.la \
	$(top_builddir)/src/waltham/libwalthamclient.la
client_CFLAGS = \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/src/waltham/ \
	-I$(top_srcdir)/include/ \
	-I$(top_srcdir)/src/
client_SOURCES = client-api-example.c

server_LDADD = \
	@GLIB_LIBS@ \
	$(top_builddir)/src/util/libwalthamutil.la \
	$(top_builddir)/src/waltham/libwalthamserver.la
server_CFLAGS = \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/src/waltham/ \
	-I$(top_srcdir)/include/ \
	-I$(top_srcdir)/src/util/ \
	-I$(top_srcdir)/src/
server_SOURCES = \
	server-api-example.c \
	w-util.c
