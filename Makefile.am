ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

SUBDIRS = extern .

bin_PROGRAMS = diffshipper
diffshipper_SOURCES = src/api.c src/buf.c src/conf.c src/dmp_lua.c src/fs_event_handlers.c src/ignore.c src/init_room.c src/mmap.c src/log.c src/net.c src/main.c src/options.c src/proto_handlers.c src/scandir.c src/util.c
diffshipper_LDADD = -ljansson -lpthread -lthriftmd5 -lcurl $(LUA_LIBS)

man_MANS = doc/diffshipper.1

# Uncomment this line to output gprof profiling info.
#CFLAGS=-pg
