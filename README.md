coroserver
==========

Multi thread TCP server with Boost.Asio and Boost.Coroutine

This program uses [Boost 1.54](http://www.boost.org/users/history/version_1_54_0.html).

* To build with GCC 4.7 or newer on Linux, or Xcode 4 on Mac OS X 10.8 or later:

  `cmake [options] path/to/source`

* `coroserver` listens on port 20000, you can use `telnet` to connect.

* Type `quit` to disconnect.