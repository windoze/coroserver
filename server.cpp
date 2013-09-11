//
//  server.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <functional>
#include <memory>
#include <thread>
#include <boost/bind.hpp>
#include <boost/asio/spawn.hpp>
#include "session.h"
#include "server.h"

server::server(const std::string &address,
               const std::string &port,
               std::size_t thread_pool_size)
: thread_pool_size_(thread_pool_size)
, io_service_()
, signals_(io_service_)
, acceptor_(io_service_)
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
    signals_.async_wait(std::bind(&server::stop, this));
    
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
    
    start();
}

void server::run() {
    // Create a pool of threads to run all of the io_services.
    std::vector<std::shared_ptr<std::thread> > threads;
    for (std::size_t i = 0; i < thread_pool_size_; ++i) {
        // NOTE: It's weird that std::bind doesn't work here
        std::shared_ptr<std::thread> thread(new std::thread(boost::bind(&boost::asio::io_service::run,
                                                                        &io_service_)));
        threads.push_back(thread);
    }
    
    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
        threads[i]->join();
}

void server::start() {
    boost::asio::spawn(io_service_,
                       [&](boost::asio::yield_context yield)
                       {
                           for (;;)
                           {
                               boost::system::error_code ec;
                               boost::asio::ip::tcp::socket socket(io_service_);
                               acceptor_.async_accept(socket, yield[ec]);
                               if (!ec)
                                   std::make_shared<session>(std::move(socket))->start();
                           }
                       });
}

void server::stop() {
    io_service_.stop();
}
