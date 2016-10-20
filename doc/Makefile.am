EXTRA_DIST = \
	doxygen.conf.in \
	api_groups.dox \
	usage.dox

if ENABLE_DOC
noinst_DATA = html/index.html
endif

public_API_files = \
	@top_builddir@/doc/mainpage.md \
	@top_srcdir@/doc/api_groups.dox \
	@top_srcdir@/doc/usage.dox \
	@top_srcdir@/src/waltham/waltham-connection.h \
	@top_srcdir@/src/waltham/waltham-object.h \
	@top_srcdir@/src/waltham/waltham-util.h \
	@top_builddir@/src/waltham/waltham-client.h \
	@top_builddir@/src/waltham/waltham-server.h

html/index.html: doxygen.conf $(public_API_files)
	$(AM_V_GEN)(cat doxygen.conf; \
	            echo "INPUT= $(public_API_files)"; \
		   ) | $(DOXYGEN) -

GH_TO_DOX_SUBSTS = \
	-e 's|^Waltham IPC Library$$|Waltham IPC Library {\#mainpage}|'

@top_builddir@/doc/mainpage.md: @top_srcdir@/README.md
	$(AM_V_GEN)$(SED) $(GH_TO_DOX_SUBSTS) < $< > $@

CLEANFILES = @top_builddir@/doc/mainpage.md

clean-local:
	rm -rf html
