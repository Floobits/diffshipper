# Diff-patch (This is a placeholder name. I welcome suggestions.)

## Development status: Not working yet. Ignore this repository.

This program monitors a directory for changed files using [FSEvents](http://en.wikipedia.org/wiki/FSEvents). When any file changes, it calculates the [operational transformation](http://en.wikipedia.org/wiki/Operational_transformation) and sends it to a remote server. Likewise, it also does the reverse: applies operational transforms sent from the remote server to local files.

## Build Instructions

Right now it only builds on OS X because of the FSEvents requirement. Sorry Linux users, I'll add inotify soon.

Dependencies: automake

    git clone https://github.com/ggreer/diff-patch-cloud.git
    cd diff-patch-cloud
    git submodule update --init
    ./autogen.sh
