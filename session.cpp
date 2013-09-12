//
//  session.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <functional>
#include <boost/asio/spawn.hpp>
#include "async_stream.h"
#include "session.h"

session::session(::boost::asio::ip::tcp::socket socket)
: socket_(std::move(socket))
, strand_(socket_.get_io_service())
{}

void session::start(std::function<bool(std::iostream&, boost::asio::yield_context)> f) {
    auto self(shared_from_this());
    boost::asio::spawn(strand_,
                       [this, self, f](boost::asio::yield_context yield) {
                           async_stream<boost::asio::ip::tcp::socket> s(socket_, yield);
                           try {
                               f(s, yield);
                           } catch ( std::exception const& e) {
                               s << "Exception caught:" << e.what() << std::endl;
                           } catch(...) {
                               s << "Unknown exception caught" << std::endl;
                           }
                       });
}
