noinst_PROGRAMS = client server capiex sapiex

capiex_LDADD = \
	@GLIB_LIBS@ \
	$(top_builddir)/src/util/libwalthamutil.la \
	$(top_builddir)/src/grayley/libgrayley.la \
	$(top_builddir)/src/waltham/libwalthamclient.la
capiex_CFLAGS = \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/src/grayley/ \
	-I$(top_srcdir)/src/waltham/ \
	-I$(top_srcdir)/include/ \
	-I$(top_srcdir)/src/
capiex_SOURCES = client-api-example.c

sapiex_LDADD = \
	@GLIB_LIBS@ \
	$(top_builddir)/src/util/libwalthamutil.la \
	$(top_builddir)/src/grayley/libgrayley.la \
	$(top_builddir)/src/waltham/libwalthamserver.la
sapiex_CFLAGS = \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/src/grayley/ \
	-I$(top_srcdir)/src/waltham/ \
	-I$(top_srcdir)/include/ \
	-I$(top_srcdir)/src/util/ \
	-I$(top_srcdir)/src/
sapiex_SOURCES = server-api-example.c \
	w-util.c

client_LDADD = \
	@GLIB_LIBS@ \
	$(top_builddir)/src/util/libwalthamutil.la \
	$(top_builddir)/src/grayley/libgrayley.la \
	$(top_builddir)/src/waltham/libwalthamclient.la
client_CFLAGS = \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/src/grayley/ \
	-I$(top_srcdir)/src/waltham/ \
	-I$(top_srcdir)/include/ \
	-I$(top_srcdir)/src/
client_SOURCES = client.c

server_LDADD = \
	@GLIB_LIBS@ \
	$(top_builddir)/src/util/libwalthamutil.la \
	$(top_builddir)/src/grayley/libgrayley.la \
	$(top_builddir)/src/waltham/libwalthamserver.la
server_CFLAGS = \
	@GLIB_CFLAGS@ \
	-I$(top_srcdir)/src/grayley/ \
	-I$(top_srcdir)/include/ \
	-I$(top_srcdir)/src/util/
server_SOURCES = server.c
