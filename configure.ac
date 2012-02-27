AC_PREREQ(2.59)
AC_INIT(libql, 0.1, [nathaniel@themccallums.org], [libsc])

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

AC_PROG_CC_STDC
AM_PROG_AS
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [
  AM_SILENT_RULES([yes])
], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

dnl Check for required dependency libsc
PKG_CHECK_MODULES([LIBSC], [libsc >= 0.2], [],
                  [AC_MSG_ERROR("libsc not found!")])

dnl Check for a supported CPU in the setjmp engine
AC_CHECK_HEADER([setjmp.h], [
  AC_MSG_CHECKING([assembly compatibility])
  case $target_cpu in
    i386|i486|i586|i686|x86_64) ASMARCH=intel;;
    arm*) ASMARCH=arm;;
    mips*) ASMARCH=mips;;
  esac
  if test -z $ASMARCH; then
    AC_MSG_WARN([not found])
    setjmp=false
  else
    AC_MSG_RESULT([$ASMARCH])
    AC_SUBST(ASMARCH)
    setjmp=true
  fi], [setjmp=false])
AM_CONDITIONAL([WITH_SETJMP], [$setjmp])

dnl Check for ucontext
AC_CHECK_HEADER([ucontext.h], [
  AC_MSG_CHECKING([ucontext functionality])
  AC_TRY_RUN([
    #include <ucontext.h>
    int main() {
      ucontext_t ctx;
      exit(getcontext(&ctx));
    }],
    [ucontext=true; AC_MSG_RESULT([works])],
    [ucontext=false; AC_MSG_RESULT([broken])])
], [ucontext=false])
AM_CONDITIONAL([WITH_UCONTEXT], [$ucontext])

dnl Check for pthread
AX_PTHREAD([pthread=true], [pthread=false])
AM_CONDITIONAL([WITH_PTHREAD], [$pthread])

dnl Generate files
AC_CONFIG_FILES(Makefile tests/Makefile libql.pc)
AC_OUTPUT

dnl Format the engines output
engines=
next=
if test $ASMARCH; then
  engines="${engines}${next}setjmp($ASMARCH)"
  next=", "
fi
if test $ucontext = true; then
  engines="${engines}${next}ucontext"
  next=", "
fi
if test $pthread = true; then
  engines="${engines}${next}pthread"
  next=", "
fi

dnl Print our result
AC_MSG_RESULT([
        $PACKAGE $VERSION
        ========

        prefix:                 ${prefix}
        libdir:                 ${libdir}
        includedir:             ${includedir}
        localstatedir:          ${localstatedir}

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
        target:                 ${target}

        engines:                ${engines}
])
