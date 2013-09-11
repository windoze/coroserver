//
//  session.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <functional>
#include <boost/asio/spawn.hpp>
#include "iobuf.h"
#include "session.h"
#include "stream_calc.h"

session::session(::boost::asio::ip::tcp::socket socket)
: socket_(std::move(socket))
, strand_(socket_.get_io_service())
{}

void session::start() {
    auto self(shared_from_this());
    boost::asio::spawn(strand_,
                       [this, self](boost::asio::yield_context yield) {
                           async_inbuf aibuf(socket_, yield);
                           std::istream sin(&aibuf);
                           
                           async_outbuf aobuf(socket_, yield);
                           std::ostream sout(&aobuf);
                           
                           try {
                               stream_calc(sin, sout);
                           } catch ( std::exception const& e) {
                               sout << "Exception caught:" << e.what() << std::endl;
                           } catch(...) {
                               sout << "Unknown exception caught" << std::endl;
                           }
                       });
}
