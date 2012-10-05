#include <pthread.h>
#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include "diff.h"
#include "log.h"
#include "net.h"
#include "util.h"


void event_cb(ConstFSEventStreamRef streamRef, void *cb_data, size_t count, void *paths,
              const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[]) {
    size_t i;
    const char *path;
    char *base_path = cb_data;

    for (i = 0; i < count; i++) {
        path = ((char**)paths)[i];
        /* flags are unsigned long, IDs are uint64_t */
        log_debug("Change %llu in %s, flags %lu", ids[i], path, (long)flags[i]);
        push_changes(base_path, path);
    }
}


void init() {
    pthread_t remote_changes;

    ignored_paths = NULL;
    ignored_paths_len = 0;
    set_log_level(LOG_LEVEL_DEBUG);
    if (pthread_cond_init(&server_conn_ready, NULL)) {
        die("pthread_cond_init failed!");
    }
    if (pthread_mutex_init(&server_conn_mtx, NULL)) {
        die("pthread_mutex_init failed!");
    }
    if (pthread_mutex_init(&ignore_mtx, NULL)) {
        die("pthread_mutex_init failed!");
    }
    pthread_create(&remote_changes, NULL, &remote_change_worker, NULL);
}


void cleanup() {
    pthread_cond_destroy(&server_conn_ready);
    pthread_mutex_destroy(&server_conn_mtx);
    pthread_mutex_destroy(&ignore_mtx);
    net_cleanup();
}


int main(int argc, char **argv) {
    int rv;
    char *path;

    init();

    if (argc < 2)
        die("No path to watch specified");
    path = realpath(argv[1], NULL);

    rv = run_cmd("mkdir -p %s%s", TMP_BASE, path);
    if (rv != 0)
        die("error creating temp directory %s", TMP_BASE);

    rv = run_cmd("cp -fr %s/ %s%s", path, TMP_BASE, path);
    if (rv != 0)
        die("error creating copying files to tmp dir %s", TMP_BASE);

    log_msg("Watching %s", path);

    CFStringRef cfs_path = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8); /* pretty sure I'm leaking this */
    CFArrayRef paths = CFArrayCreate(NULL, (const void **)&cfs_path, 1, NULL); /* ditto */
    FSEventStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.info = path;
    FSEventStreamRef stream;
    CFAbsoluteTime latency = 0.25;

    stream = FSEventStreamCreate(NULL, &event_cb, &ctx, paths, kFSEventStreamEventIdSinceNow, latency, kFSEventStreamCreateFlagNone);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);

    rv = server_connect("127.0.0.1", "3148");
    if (rv != 0)
        die("Couldn't connect to server");

    CFRunLoopRun();
    /* We never get here */
    free(path);
    cleanup();

    return(0);
}
