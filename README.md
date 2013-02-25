coroserver
==========

Multi thread TCP server with Boost.Asio and Boost.Coroutine

This program uses [Boost 1.53](http://www.boost.org/users/history/version_1_53_0.html).

* To build with GCC 4.7 or newer on Linux:

  `CXXFLAGS="-std=c++11" cmake [options] path/to/source`

* To build with LLVM/Clang 3.1 or newer on Mac OS X:

  `CXXFLAGS="-std=c++11 -stdlib=libc++" LDFLAGS="-stdlib=libc++" cmake [options] path/to/source`
