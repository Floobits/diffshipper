ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = diffshipper
diffshipper_SOURCES = src/buf.c src/diff.c src/mmap.c src/log.c src/net.c src/main.c src/options.c src/proto_handlers.c src/scandir.c src/util.c
diffshipper_LDADD = -ldmpc -ljansson -lpthread
#dist_bin_SCRIPTS = auto_rsync notify_loop

#man_MANS = doc/diffshipper.1

# Uncomment this line to output gprof profiling info.
#CFLAGS=-pg
