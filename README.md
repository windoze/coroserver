coroserver
==========

Multi thread socket server with Boost.Asio and Boost.Coroutine

This program uses [Boost 1.55](http://www.boost.org/users/history/version_1_55_0.html) and [http-parser](https://github.com/joyent/http-parser) from joyent.

* The project needs C++11 compliant compiler and standard lib, it has been tested on:

  + Clang 3.3 with libc++ on FreeBSD 10 BETA3
  
  + GCC 4.8 on Ubuntu Linux 13.10

  + Xcode 5/Clang 3.3 with libc++ on Mac OS X 10.9

* To build on FreeBSD or Linux:

  + Run `git submodule update --init` in source directory

  + Run `cmake [options] path/to/source`
  
* To build on Mac OS X 10.9

  + Use Xcode 5, or
  
  + Follow the building process on FreeBSD/Linux.

* `coroserver` listens on port 20000 with a HTTP server, 20001 with a half-worked HTTP proxy, and 30000 with a line-oriented calculator.
