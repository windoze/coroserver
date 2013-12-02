//
//  server.cpp
//  coroserver
//
//  Created by Windoze on 13-2-24.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <thread>
#include <boost/asio/spawn.hpp>
#include "server.h"

using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

server::server(protocol_handler_t &&protocol_processor,
               const endpoint_list_t &endpoints,
               std::size_t thread_pool_size)
: protocol_processor_(std::move(protocol_processor))
, thread_pool_size_(thread_pool_size)
, io_service_()
, signals_(io_service_)
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
    signals_.async_wait([this](system::error_code, int){ close(); });
    
    // Start listening on endpoints
    listen(endpoints);
    
    // Accept incomint connections
    open();
}

void server::listen(const endpoint_list_t &epl) {
    for (const endpoint_t &ep : epl)
        listen(ep);
}

void server::listen(const endpoint_t &ep) {
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    tcp::resolver resolver(io_service_);
    tcp::resolver::query query(ep.first, ep.second);
    tcp::endpoint endpoint = *resolver.resolve(query);
    acceptor_list_t::iterator i=acceptors_.emplace(acceptors_.end(),
                                                   tcp::acceptor(io_service_));
    i->open(endpoint.protocol());
    i->set_option(tcp::acceptor::reuse_address(true));
    i->bind(endpoint);
    i->listen();
}

void server::run() {
    // Create a pool of threads to run all of the io_services.
    std::vector<std::thread> threads;
    for (std::size_t i=0; i<thread_pool_size_; ++i) {
        threads.emplace(threads.end(),
                        std::thread([this](){io_service_.run();}));
    }
    
    // Wait for all threads in the pool to exit.
    for (std::size_t i=0; i<threads.size(); ++i)
        threads[i].join();
}

void server::open() {
    for (tcp::acceptor &a : acceptors_) {
        spawn(io_service_,
              [&](yield_context yield) {
                  for (;;) {
                      system::error_code ec;
                      tcp::socket socket(io_service_);
                      a.async_accept(socket, yield[ec]);
                      if (!ec)
                          handle_connect(std::move(socket));
                  }
              });
    }
}

void server::close()
{ io_service_.stop(); }

void server::handle_connect(tcp::socket &&socket) {
    spawn(strand(io_service_),
          [this, &socket](yield_context yield) {
              async_tcp_stream s(std::move(socket), yield);
              try {
                  protocol_processor_(s);
              } catch (std::exception const& e) {
                  // TODO: Log error
              } catch(...) {
                  // TODO: Log error
              }
          });
}
