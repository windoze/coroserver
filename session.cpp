//
//  session.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <boost/bind.hpp>
#include "iobuf.h"
#include "session.h"
#include "stream_calc.h"

session::session(boost::asio::io_service &io_service)
: coro_()
, io_service_(io_service)
, socket_(io_service_)
{}

void session::start() {
    coro_ = coro_t( boost::bind(&session::handle_read, this, _1) );
}

void session::handle_read(coro_t::caller_type &ca) {
    inbuf ibuf(socket_, coro_, ca);
    std::istream sin(&ibuf);

    outbuf obuf(socket_, coro_, ca);
    std::ostream sout(&obuf);

    stream_calc(sin, sout);
    io_service_.post( boost::bind(&session::destroy, this) );
}

void session::destroy() {
    delete this;
}
