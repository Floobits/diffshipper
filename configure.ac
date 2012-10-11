AC_INIT([diffshipper], [0.1], [https://github.com/ggreer/diffshipper/issues], [diffshipper], [https://github.com/ggreer/diffshipper])
AC_PROG_CC
AC_PREREQ([2.59])

AM_INIT_AUTOMAKE([no-define foreign subdir-objects])

m4_ifdef(
    [AM_SILENT_RULES],
    [AM_SILENT_RULES([yes])]
)

CFLAGS="$CFLAGS -Iextern/diff-match-patch-c/src/"
LDFLAGS="$LDFLAGS -Lextern/diff-match-patch-c/"

AC_CHECK_HEADERS([pthread.h])

AC_CHECK_HEADERS([CoreFoundation/CoreFoundation.h], [
    CFLAGS="$CFLAGS -I /System/Library/Frameworks/CoreFoundation.framework/Headers"
    LDFLAGS="$LDFLAGS -framework CoreFoundation -framework CoreServices"
    AC_DEFINE([FSEVENTS], [1], [FSEvents exists])
    AC_DEFINE([FS_WATCHER], [1], [FSEvents or Inotify exists])
    ],
    [],
    []
)

AC_CHECK_HEADERS([sys/inotify.h], [
    LIBS="$LIBS -linotifytools"
    AC_DEFINE([INOTIFY], [1], [Inotify exists])
    AC_DEFINE([FS_WATCHER], [1], [FSEvents or Inotify exists])
    ],
    [],
    []
)

CFLAGS="$CFLAGS -Wall -Wextra -D_GNU_SOURCE -g"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_HEADERS([src/config.h])

AC_OUTPUT
