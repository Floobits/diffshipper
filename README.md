# Diffshipper

For OS X and Linux.

## Development status: Not working yet. Ignore this repository.

This program monitors a directory for changed files using [FSEvents](http://en.wikipedia.org/wiki/FSEvents) or [inotify](http://en.wikipedia.org/wiki/Inotify). When any file changes, it calculates the [operational transformation](http://en.wikipedia.org/wiki/Operational_transformation) and sends it to a remote server. Likewise, it also does the reverse: applies operational transforms sent from the remote server to local files.

## Build Instructions

Dependencies: automake, [Jansson](http://www.digip.org/jansson/), pthreads. Linux users also need libinotifytools-dev.

    git clone https://github.com/ggreer/diffshipper.git
    cd diffshipper
    git submodule update --init
    ./autogen.sh

## TODO

* Protocol stuff
* Initial data sync
