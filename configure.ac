AC_INIT([diffmon], [0.1])
AC_PROG_CC
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([no-define foreign subdir-objects])

m4_ifdef(
    [AM_SILENT_RULES],
    [AM_SILENT_RULES([yes])]
)

CFLAGS="$CFLAGS -Iextern/diff-match-patch-c/src/ -Wall -Wextra -std=c89 -D_GNU_SOURCE -g"
LDFLAGS="$LDFLAGS -framework CoreFoundation -framework CoreServices"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([config.h])

AC_OUTPUT
