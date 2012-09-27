#include <stdlib.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include "diff.h"
#include "log.h"
#include "util.h"

void event_cb(ConstFSEventStreamRef streamRef, void *cb_data, size_t count, void *paths,
              const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[]) {
    size_t i;
    const char *path;

    for (i = 0; i < count; i++) {
        path = ((char**)paths)[i];
        /* flags are unsigned long, IDs are uint64_t */
        log_debug("Change %llu in %s, flags %lu", ids[i], path, (long)flags[i]);
        push_changes(path);
    }

    if (count > 0) {
/*        exit(1);*/
    }
}

int main(int argc, char **argv) {
    int rv;
    char *path;

    set_log_level(LOG_LEVEL_DEBUG);

    if (argc < 2)
        die("No path to watch specified");
    path = realpath(argv[1], NULL);

    rv = run_cmd("mkdir -p %s%s", TMP_BASE, path);
    if (rv != 0)
        die("error creating temp directior %s", TMP_BASE);

    rv = run_cmd("cp -fr %s/ %s%s", path, TMP_BASE, path);
    if (rv != 0)
        die("error creating copying files to tmp dir %s", TMP_BASE);

    log_msg("Watching %s", path);
    free(path);

    CFStringRef cfs_path = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8); /* pretty sure I'm leaking this */
    CFArrayRef paths = CFArrayCreate(NULL, (const void **)&cfs_path, 1, NULL); /* ditto */
    void *cb_data = NULL;
    FSEventStreamRef stream;

    stream = FSEventStreamCreate(NULL, &event_cb, cb_data, paths, kFSEventStreamEventIdSinceNow, 0, kFSEventStreamCreateFlagNone);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);

    CFRunLoopRun();
    /* We never get here */

    return(0);
}
