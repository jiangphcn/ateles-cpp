# Ateles

The new javascript worker runtime for CouchDB on FDB.

## Building

Once all dependencies are installed (see below) building ateles is
as simple as:

```bash
  make
```

## Dependencies

### macOS

```bash
  brew install cmake
  brew install grpc
```

## Installing spidermonkey

### macOS

```bash
  brew install autoconf@2.13
  brew install yasm
  mkdir ~/tmp/spidermonkey && cd ~/tmp/spidermonkey
  wget http://ftp.mozilla.org/pub/spidermonkey/prereleases/52/pre1/mozjs-52.9.1pre1.tar.bz2
  tar -xvjf mozjs-52.9.1pre1.tar.bz2
  cd mozjs-52.9.1pre1/js/src
  mkdir obj
  cd obj
  CXXFLAGS=-stdlib=libc++ ../configure
  make -j4
  make install
  cp ./mozglue/build/libmozglue.dylib /usr/local/lib/
```

### Linux

```bash
  sudo apt-get install autoconf2.13
  sudo apt-get install yasm
  mkdir ~/tmp/spidermonkey && cd ~/tmp/spidermonkey
  wget http://ftp.mozilla.org/pub/spidermonkey/prereleases/52/pre1/mozjs-52.9.1pre1.tar.bz2
  tar -xvjf mozjs-52.9.1pre1.tar.bz2
  cd mozjs-52.9.1pre1/js/src
  mkdir obj
  cd obj
  ../configure
  make -j4
  make install
  cp ./mozglue/build/libmozglue.dylib /usr/local/lib/
```
