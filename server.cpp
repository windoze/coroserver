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

server::server(const sap_desc_list_t &sap_desc_list,
               std::size_t thread_pool_size)
: thread_pool_size_(thread_pool_size)
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
    for (const sap_desc_t &sd : sap_desc_list)
        listen(sd);
    
    // Accept incomint connections
    open();
}

void server::listen(const sap_desc_t &sd) {
    const endpoint_t &ep=sd.second;
    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    tcp::resolver resolver(io_service_);
    tcp::resolver::query query(ep.first, ep.second);
    tcp::endpoint endpoint = *resolver.resolve(query);
    sap_ptr sap(new sap_t(sd.first, tcp::acceptor(io_service_)));
    saps_.push_back(sap);
    sap_list_t::reverse_iterator i=saps_.rbegin();
    (*i)->second.open(endpoint.protocol());
    (*i)->second.set_option(tcp::acceptor::reuse_address(true));
    (*i)->second.bind(endpoint);
    (*i)->second.listen();
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
    for (sap_ptr &sap : saps_) {
        spawn(io_service_,
              [&](yield_context yield) {
                  for (;;) {
                      system::error_code ec;
                      tcp::socket socket(io_service_);
                      sap->second.async_accept(socket, yield[ec]);
                      if (!ec)
                          handle_connect(std::move(socket), sap->first);
                  }
              });
    }
}

void server::close()
{ io_service_.stop(); }

void server::handle_connect(tcp::socket &&socket, const protocol_handler_t &handler) {
    spawn(strand(io_service_),
          // Create a new protocol handler for each connection
          [this, &socket, handler](yield_context yield) {
              std::shared_ptr<async_tcp_streambuf> sbp(std::make_shared<async_tcp_streambuf>(std::move(socket), yield));
              async_tcp_stream s(sbp);
              try {
                  handler(s);
              } catch (std::exception const& e) {
                  // TODO: Log error
              } catch(...) {
                  // TODO: Log error
              }
          });
}
