dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libql, 0.1)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

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

AC_CHECK_HEADER([ucontext.h], [ucontext=true], [ucontext=false])
AM_CONDITIONAL([WITH_UCONTEXT], [$ucontext])

AC_CHECK_LIB([pthread], [pthread_create],
             [AC_CHECK_HEADER([pthread.h],
                              [pthread=true],
                              [pthread=false])],
             [pthread=false])
AM_CONDITIONAL([WITH_PTHREAD], [$pthread])

AC_MSG_CHECKING([assembly compatibility])
case $target_cpu in
  i386|i486|i586|i686|x86_64) ASMARCH=intel;;
  arm*) ASMARCH=arm;;
esac
if test x$ASMARCH == x; then
  AC_MSG_WARN([not found])
  AM_CONDITIONAL([WITH_ASSEMBLY], [false])
else
  AC_MSG_RESULT([$ASMARCH])
  AM_CONDITIONAL([WITH_ASSEMBLY], [true])
  AC_SUBST(ASMARCH)
fi 

AC_MSG_NOTICE()
AC_MSG_NOTICE(AS_HELP_STRING([CC], [$CC]))
AC_MSG_NOTICE(AS_HELP_STRING([CFLAGS], [$CFLAGS]))
AC_MSG_NOTICE(AS_HELP_STRING([TARGET], [$target]))
AC_MSG_NOTICE()

AC_MSG_NOTICE([ENGINES:])
if test $ASMARCH; then
AC_MSG_NOTICE(AS_HELP_STRING([assembly], [$ASMARCH]))
else
AC_MSG_NOTICE(AS_HELP_STRING([assembly], [false]))
fi
AC_MSG_NOTICE(AS_HELP_STRING([ucontext], [$ucontext]))
AC_MSG_NOTICE(AS_HELP_STRING([pthread], [$pthread]))
AC_MSG_NOTICE()

AC_CONFIG_FILES(Makefile libql.pc)
AC_OUTPUT
