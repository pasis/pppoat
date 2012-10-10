AC_INIT([pppoat], [dev], [pasis.ua@gmail.com])
AM_INIT_AUTOMAKE

AC_PROG_CC
AM_PROG_CC_C_O

AC_ARG_ENABLE([xmpp], [AS_HELP_STRING([--enable-xmpp], [enable xmpp module])])

# check dependencies
if test "x$enable_xmpp" = xyes; then
  PKG_CHECK_MODULES([libstrophe], [libstrophe >= 0.8],
	[],
	[AC_MSG_ERROR([libstrophe is required for xmpp module])])
fi

# 
if test "x$enable_xmpp" = xyes; then
  LIBS="$LIBS $libstrophe_LIBS"
  AC_MSG_NOTICE([module xmpp will be build])
else
  AC_MSG_NOTICE([module xmpp will NOT be build])
fi

AM_CONDITIONAL([MOD_XMPP], [test "x$enable_xmpp" = xyes])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
