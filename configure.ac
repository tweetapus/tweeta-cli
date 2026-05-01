AC_INIT([tweeta-cli], [0.1.0], [https://github.com/tweetapus/tweeta-cli/issues])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_FILES([Makefile])

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_INSTALL

AS_IF([test "${ac_cv_env_CFLAGS_set}" != set], [
  CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -O2"
])

AC_ARG_VAR([PKG_CONFIG], [path to pkg-config])
AC_ARG_VAR([LIBCURL_CFLAGS], [C compiler flags for libcurl])
AC_ARG_VAR([LIBCURL_LIBS], [linker flags for libcurl])

AC_PATH_TOOL([PKG_CONFIG], [pkg-config])

AS_IF([test "x$LIBCURL_CFLAGS" = x && test -n "$PKG_CONFIG"], [
  AS_IF([$PKG_CONFIG --exists libcurl], [
    LIBCURL_CFLAGS=`$PKG_CONFIG --cflags libcurl`
  ])
])

AS_IF([test "x$LIBCURL_LIBS" = x && test -n "$PKG_CONFIG"], [
  AS_IF([$PKG_CONFIG --exists libcurl], [
    LIBCURL_LIBS=`$PKG_CONFIG --libs libcurl`
  ])
])

AS_IF([test "x$LIBCURL_LIBS" = x], [
  LIBCURL_LIBS="-lcurl"
])

save_CPPFLAGS=$CPPFLAGS
save_LIBS=$LIBS
CPPFLAGS="$CPPFLAGS $LIBCURL_CFLAGS"
LIBS="$LIBCURL_LIBS $LIBS"
AC_CHECK_HEADER([curl/curl.h], [], [
  AC_MSG_ERROR([curl/curl.h not found; install libcurl development headers or set LIBCURL_CFLAGS])
])
AC_LINK_IFELSE(
  [AC_LANG_PROGRAM([[#include <curl/curl.h>]], [[curl_global_init(CURL_GLOBAL_DEFAULT);]])],
  [],
  [AC_MSG_ERROR([libcurl link test failed; install libcurl or set LIBCURL_LIBS])]
)
CPPFLAGS=$save_CPPFLAGS
LIBS=$save_LIBS

AC_SUBST([LIBCURL_CFLAGS])
AC_SUBST([LIBCURL_LIBS])

AC_OUTPUT
