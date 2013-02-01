# Floobits Diff Shipper

For OS X and Linux.

## Development status: Usable, but still has a few bugs.

This program monitors a directory for changed files using [FSEvents](http://en.wikipedia.org/wiki/FSEvents) or [inotify](http://en.wikipedia.org/wiki/Inotify). When any file changes, it calculates the [operational transformation](http://en.wikipedia.org/wiki/Operational_transformation) and sends it to a remote server. Likewise, it also does the reverse: applies operational transformations sent from the remote server to local files.


## Build Instructions

### Dependencies
To build, you'll need automake, [Jansson](http://www.digip.org/jansson/)>=2.4, [libcurl](http://curl.haxx.se/libcurl/), pthreads, Lua 5.2 or later, and pkg-config. Linux users need libinotifytools-dev.

#### OS X

    brew install automake jannson pkg-config

Since homebrew only has Lua 5.1 right now, OS X users will have to download and install Lua from source.

#### Ubuntu

    apt-get install -y automake build-essential libcurl4-openssl-dev libinotifytools-dev libjansson-dev pkg-config

### Building

    git clone https://github.com/Floobits/diffshipper.git
    cd diffshipper
    ./autogen.sh

Building requires Jansson version 2.4 or greater. If you have an older version of Ubuntu, `apt-get` might install an incompatible version of Jansson. If this happens, you'll have to `apt-get purge libjansson-dev` and install [Jansson](http://www.digip.org/jansson/) from source.


## Usage

Running the diffshipper requires a few arguments: `-r` is the room name, `-o` is the room owner, `-u` is your Floobits username, and `-s` is your Floobits API secret. You can find (and reset) your secret on [your settings page](https://floobits.com/dash/settings/).

### Join a room

    diffshipper -r testing -o ownername -u myuser -s gii9Ka8aZei3ej1eighu2vi8D ~/code/repo

### Create a room from an existing directory

    diffshipper --create-room --room-perms 2 -r testing -o ownername -u myuser -s gii9Ka8aZei3ej1eighu2vi8D ~/code/repo

### Stomp over an existing room

    diffshipper --recreate-room --room-perms 2 -r testing -o ownername -u myuser -s gii9Ka8aZei3ej1eighu2vi8D ~/code/repo


`man diffshipper` for more info and examples.
