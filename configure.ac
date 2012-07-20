#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([lvdbg], [0.0.1], [github.com/klo99/lvdbg])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])
AM_PATH_CHECK

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

AC_CHECK_PROGS(GCOV, gcov, false)
AC_CHECK_PROGS(LCOV, lcov, false)
AC_CHECK_PROGS(GENHTML, genhtml, false)

# Checks for libraries.
AC_CHECK_LIB([ncurses], [initscr])
AC_CHECK_LIB([panel], [new_panel])
AC_CHECK_LIB([util], [forkpty])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h unistd.h termios.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_UINT64_T
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([getcwd memset select strcasecmp strchr strdup strerror strstr strtol strtoul])

# README-alpha
changequote(,)dnl
case $VERSION in
  [0-9]*.[0-9]*[a-z]) DIST_ALPHA="README-alpha";;
  [0-9]*.[0-9]*.[0-9]*) DIST_ALPHA="README-alpha";;
  *) DIST_ALPHA=;;
esac
changequote([, ])dnl
AC_SUBST(DIST_ALPHA)

# Configure options

AC_ARG_ENABLE(gcov,
AC_HELP_STRING([--enable-gcov],
         [turn on test coverage @<:@default=no@:>@]),
[case "${enableval}" in
  yes) enable_gcov=true ;;
  no)  enable_gcov=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-gcov) ;;
esac], [enable_gcov=false ])

if test x$enable_gcov = xtrue ; then
  GCOV_CFLAGS="-fprofile-arcs -ftest-coverage -g"
  AC_SUBST(GCOV_CFLAGS)

  dnl libtool 1.5.22 and lower strip -fprofile-arcs from the flags
  dnl passed to the linker, which is a bug; -fprofile-arcs implicitly
  dnl links in -lgcov, so we do it explicitly here for the same effect
  GCOV_LIBS=-lgcov
  AC_SUBST(GCOV_LIBS)
fi
AM_CONDITIONAL(ENABLE_GCOV, test x"$enable_gcov" = "xtrue")

AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug],
  [Enable debug messages @<:@default no@:>@]),,enable_debug=no)
AC_MSG_CHECKING([whether to enable debugging mode])
if test x$enable_debug = xyes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(DEBUG, 1, [Enable debug mode])
  DEBUG_FLAGS="-DDEBUG -g"
else
  AC_MSG_RESULT(no)
  DEBUG_FLAGS="-DNDEBUG"
fi
AC_SUBST(DEBUG_FLAGS)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/lvdbg.h
                 docs/examples/Makefile
                 docs/Makefile
                 docs/Doxyfile
                 tests/Makefile
		 data/Makefile])
AC_OUTPUT
