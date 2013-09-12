//
//  session.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <functional>
#include <boost/asio/spawn.hpp>
#include "async_streambuf.h"
#include "session.h"
#include "http_session.h"

session::session(::boost::asio::ip::tcp::socket socket)
: socket_(std::move(socket))
, strand_(socket_.get_io_service())
{}

void session::start() {
    auto self(shared_from_this());
    boost::asio::spawn(strand_,
                       [this, self](boost::asio::yield_context yield) {
                           async_streambuf<boost::asio::ip::tcp::socket> sbuf(socket_, yield);
                           std::iostream s(&sbuf);
                           
                           try {
                               http_session sn(s, yield);
                               sn();
                           } catch ( std::exception const& e) {
                               s << "Exception caught:" << e.what() << std::endl;
                           } catch(...) {
                               s << "Unknown exception caught" << std::endl;
                           }
                       });
}
