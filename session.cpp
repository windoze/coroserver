//
//  session.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <functional>
#include "iobuf.h"
#include "session.h"
#include "stream_calc.h"

session::session(boost::asio::io_service &io_service)
: coro_()
, socket_(io_service)
, strand_(socket_.get_io_service())
{}

void session::start() {
    boost::asio::spawn(strand_, std::bind(&session::go, this, std::placeholders::_1));
}

void session::go(boost::asio::yield_context yield) {
    inbuf ibuf(socket_, yield);
    std::istream sin(&ibuf);

    outbuf obuf(socket_, yield);
    std::ostream sout(&obuf);

    try {
        stream_calc(sin, sout);
    } catch( boost::coroutines::detail::forced_unwind) {
        throw;
    } catch ( std::exception const& e) {
        sout << "Exception caught:" << e.what() << std::endl;
    } catch(...) {
        sout << "Unknown exception caught" << std::endl;
    }

    strand_.post( std::bind(&session::destroy, this) );
}

void session::destroy() {
    delete this;
}
