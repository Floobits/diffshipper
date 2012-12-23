# Floobits Diff Shipper

For OS X and Linux.

## Development status: Only syncs in one direction. Ignore this repository for now.

This program monitors a directory for changed files using [FSEvents](http://en.wikipedia.org/wiki/FSEvents) or [inotify](http://en.wikipedia.org/wiki/Inotify). When any file changes, it calculates the [operational transformation](http://en.wikipedia.org/wiki/Operational_transformation) and sends it to a remote server. Likewise, it also does the reverse: applies operational transformations sent from the remote server to local files.

## Build Instructions

Dependencies: automake, [Jansson](http://www.digip.org/jansson/), pthreads. Linux users also need libinotifytools-dev.

    apt-get install build-essential automake libjansson-dev libinotifytools-dev

    git clone https://github.com/Floobits/diffshipper.git
    cd diffshipper
    git submodule update --init
    ./autogen.sh

## TODO

* Apply patches sent from the server
* Calculate patches from diffs
