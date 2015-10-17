AC_INIT([pppoat], [dev], [pasis.ua@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])

AC_CONFIG_FILES([Makefile])

AC_PROG_CC
AM_PROG_CC_C_O

AC_ARG_ENABLE([xmpp], [AS_HELP_STRING([--disable-xmpp], [disable xmpp module])])

if test "x$enable_xmpp" != xno; then
  PKG_CHECK_MODULES([libstrophe], [libstrophe >= 0.8.9],
	[],
	[AC_MSG_ERROR([libstrophe is required for xmpp module])])

  LIBS="$libstrophe_LIBS $LIBS"
  CFLAGS="$CFLAGS $libstrophe_CFLAGS"
fi

AC_OUTPUT
