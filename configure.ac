AC_INIT([amhello], [1.0], [bug-automake@gnu.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PREREQ([2.65])
AC_PROG_CXX
AC_PROG_CC

AC_CHECK_FUNCS([socket])
AC_TYPE_SIZE_T

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 echo/Makefile
 daytime/Makefile
])
AC_OUTPUT

