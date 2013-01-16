# Floobits Diff Shipper

For OS X and Linux.

## Development status: Unstable. Incomplete. Not ready for general use.

This program monitors a directory for changed files using [FSEvents](http://en.wikipedia.org/wiki/FSEvents) or [inotify](http://en.wikipedia.org/wiki/Inotify). When any file changes, it calculates the [operational transformation](http://en.wikipedia.org/wiki/Operational_transformation) and sends it to a remote server. Likewise, it also does the reverse: applies operational transformations sent from the remote server to local files.


## Build Instructions

### Dependencies
To build, you'll need automake, [Jansson](http://www.digip.org/jansson/)>=2.4, [libcurl](http://curl.haxx.se/libcurl/), pthreads, and pkg-config. Linux users need libinotifytools-dev. Diffshipper also relies on [diff-match-patch-c](https://github.com/ggreer/diff-match-patch-c), but it comes bundled as a submodule in extern.

#### OS X

    brew install automake jannson pkg-config

#### Ubuntu

    apt-get install -y automake build-essential libcurl4-openssl-dev libinotifytools-dev libjansson-dev pkg-config

### Building

    git clone https://github.com/Floobits/diffshipper.git
    cd diffshipper
    git submodule update --init
    ./autogen.sh

Building requires Jansson version 2.4 or greater. If you have an older version of Ubuntu, `apt-get` might install an incompatible version of Jansson. If this happens, you'll have to `apt-get purge libjansson-dev` and install [Jansson](http://www.digip.org/jansson/) from source.


## Usage

Running the diffshipper requires certain arguments: `-r` is the room name, `-o` is the room owner, `-u` is your Floobits username, and `-s` is your Floobits API secret. You can find (and reset) your secret on [your settings page](https://floobits.com/dash/settings/).

### Join a room

    diffshipper -r test -o ownername -u myusername -s 6kht3k8t4 /path/to/where/files/are/saved

### Create a room from an existing directory

    diffshipper --create-room --room-perms 2 -r new_room -o myusername -u myusername -s 6kht3k8t4 ~/code/project

`man diffshipper` for more info and examples.
