dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libql, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([1.11])

AC_PROG_CC
AM_PROG_AS
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [
  AM_SILENT_RULES([yes])
], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
