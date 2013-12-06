coroserver
==========

Multi thread HTTP server with Boost.Asio and Boost.Coroutine

This program uses [Boost 1.54](http://www.boost.org/users/history/version_1_54_0.html) and [http-parser](https://github.com/joyent/http-parser) from joyent.

* The project needs C++11 compliant compiler and standard lib, it has been tested on:

  + Clang 3.3 with libc++ on FreeBSD 10 BETA3
  
  + GCC 4.7/4.8 on Linux

  + Xcode 5/Clang 3.3 with libc++ on Mac OS X 10.9

* To build with GCC 4.7 or newer on Linux, or Xcode 5 on Mac OS X 10.9 or later:

  + Run `git submodule update --init` in source directory

  + Run `cmake [options] path/to/source`

* `coroserver` listens on port 20000
