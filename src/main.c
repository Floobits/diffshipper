#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#include "diff.h"
#include "log.h"


void event_cb(ConstFSEventStreamRef streamRef, void *cb_data, size_t count, void *paths,
              const FSEventStreamEventFlags flags[], const FSEventStreamEventId ids[]) {
    size_t i;
    const char *path;

    for (i = 0; i < count; i++) {
        path = ((char**)paths)[i];
        /* flags are unsigned long, IDs are uint64_t */
        log_debug("Change %llu in %s, flags %lu", ids[i], path, (long)flags[i]);
        get_changes(path);
    }

    if (count > 0) {
        exit(1);
    }
}

int main(int argc, char **argv) {
    set_log_level(LOG_LEVEL_DEBUG);

    if (argc < 2) {
        log_err("No path to watch specified");
        exit(1);
    }

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
