AC_PREREQ([2.64])

AC_INIT([waltham], [0.1.0])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

# Check for programs
AC_PROG_CC
AC_PROG_CC_STDC
AC_CHECK_PROGS([PYTHON2], [python2 python])
PKG_PROG_PKG_CONFIG()

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.30])

AC_ARG_ENABLE(gprof,
  AS_HELP_STRING([--enable-gprof=@<:@no/yes@:>@], [build with support for gprof]),,
    [enable_gprof=no])
if test "x$enable_gprof" != "xno" ; then
    CFLAGS="$CFLAGS -pg"
    LDFLAGS="$LDFLAGS -pg"
fi

AC_SUBST(LDFLAGS)

if test "x$GCC" = "xyes"; then
	GCC_CFLAGS="-Wall -Wextra -Wno-format-zero-length -Wno-unused-parameter -Wstrict-prototypes -Wmissing-prototypes -Wno-missing-field-initializers -fvisibility=hidden"
fi
AC_SUBST(GCC_CFLAGS)

AC_CONFIG_FILES([Makefile
		 data/Makefile
		 data/waltham.pc
		 include/Makefile
		 src/Makefile
		 src/waltham/Makefile
		 tools/Makefile
		 tests/Makefile
		])
AC_OUTPUT
