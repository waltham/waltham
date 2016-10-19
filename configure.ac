AC_PREREQ([2.64])

AC_INIT([Waltham], [0.1.0])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

dnl https://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
dnl when adding API: bump current and age, reset revision to zero
dnl when breaking API: bump current, reset age and revision to zero
dnl the rest of the time: bump revision
CURRENT=0
REVISION=0
AGE=0

VERSION_INFO="$CURRENT:$REVISION:$AGE"
AC_SUBST(VERSION_INFO)

# Check for programs
AC_PROG_CC
AC_PROG_CC_STDC
AC_CHECK_PROGS([PYTHON2], [python2 python])
PKG_PROG_PKG_CONFIG()

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

AC_ARG_ENABLE(doc,
	      AS_HELP_STRING([--enable-doc],
			     [Documentation with Doxygen @<:@default=auto@:>@]),,
	      enable_doc=auto)
if test "x$enable_doc" != "xno"; then
	AC_CHECK_PROGS([DOXYGEN], [doxygen])
	if test "x$DOXYGEN" = "x" -a "x$enable_doc" = "xyes"; then
		AC_MSG_ERROR([Documentation requested, but Doxygen not found.])
	fi
	if test "x$DOXYGEN" != "x"; then
		AC_MSG_CHECKING([doxygen is at least version 1.8])
		doxygen_version=`$DOXYGEN --version`
		AS_VERSION_COMPARE([$doxygen_version], [1.8.0],
		                   [doxygen_ok=no], [doxygen_ok=yes],
		                   [doxygen_ok=yes])
		AC_MSG_RESULT([$doxygen_ok])

		if test "x$doxygen_ok" != "xyes" -a "x$enable_doc" = "xyes"; then
			AC_MSG_ERROR([Doxygen $doxygen_version is too old.])
		fi

		enable_doc=$doxygen_ok
	else
		enable_doc=no
	fi
fi
AM_CONDITIONAL(ENABLE_DOC, test "x$enable_doc" = "xyes")

AC_CONFIG_FILES([Makefile
		 data/Makefile
		 data/waltham.pc
		 doc/doxygen.conf
		 doc/Makefile
		 src/Makefile
		 src/waltham/Makefile
		 tools/Makefile
		 tests/Makefile
		])
AC_OUTPUT
