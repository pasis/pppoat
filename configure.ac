AC_INIT([pppoat], [dev], [pasis.ua@gmail.com])
AM_INIT_AUTOMAKE

AC_PROG_CC
AM_PROG_CC_C_O

AC_ARG_ENABLE([xmpp], [AS_HELP_STRING([--enable-xmpp], [enable xmpp module])])
AC_ARG_ENABLE([loop], [AS_HELP_STRING([--enable-loop], [enable loop module])])
AC_ARG_ENABLE([sample], [AS_HELP_STRING([--enable-sample], [enable sample module])])

AC_ARG_WITH([pthreads],
	[AS_HELP_STRING([--without-pthreads], [disable check for pthread.h])])

# check dependencies
if test "x$with_pthreads" != xno; then
  AC_CHECK_LIB(pthread, pthread_create, [], [AC_MSG_ERROR([pthread support is required])])
fi

if test "x$enable_xmpp" != xno; then
  PKG_CHECK_MODULES([libstrophe], [libstrophe >= 0.8],
	[],
	[AC_MSG_ERROR([libstrophe is required for xmpp module])])
fi

# 
if test "x$enable_xmpp" != xno; then
  PPPOAT_LIBS="$PPPOAT_LIBS $libstrophe_LIBS"
fi

AM_CONDITIONAL([MOD_XMPP], [test "x$enable_xmpp" != xno])
AM_CONDITIONAL([MOD_LOOP], [test "x$enable_loop" = xyes])
AM_CONDITIONAL([MOD_SAMPLE], [test "x$enable_sample" = xyes])
AC_SUBST(PPPOAT_LIBS)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
