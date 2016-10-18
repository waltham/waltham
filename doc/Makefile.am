EXTRA_DIST = \
	doxygen.conf.in \
	api_groups.dox

if ENABLE_DOC
noinst_DATA = html/index.html
endif

public_API_files = \
	@top_srcdir@/doc/api_groups.dox \
	@top_srcdir@/src/waltham/waltham-connection.h \
	@top_srcdir@/src/waltham/waltham-object.h \
	@top_srcdir@/src/waltham/waltham-util.h \
	@top_builddir@/src/waltham/waltham-client.h \
	@top_builddir@/src/waltham/waltham-server.h

html/index.html: doxygen.conf $(public_API_files)
	$(AM_V_GEN)(cat doxygen.conf; \
	            echo "INPUT= $(public_API_files)"; \
		   ) | $(DOXYGEN) -

clean-local:
	rm -rf html
