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

namespace net {
    using namespace boost;
    using namespace boost::asio;
    using namespace boost::asio::ip;
    
    server::server(const sap_desc_list_t &sap_desc_list,
                   const initialization_handler_t &initialization_handler,
                   const finalization_handler_t &finalization_handler,
                   std::size_t thread_pool_size)
    : thread_pool_size_(thread_pool_size)
    , io_service_()
    , initialization_handler_(initialization_handler)
    , finalization_handler_(finalization_handler)
    , init_state_(initialization_handler_(io_service_))
    , signals_(io_service_)
    {
        if (init_state_) {
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
    }
    
    
    server::server(const sap_desc_list_t &sap_desc_list,
                   std::size_t thread_pool_size)
    : server(sap_desc_list,
             [](boost::asio::io_service &)->bool { return true; },
             [](boost::asio::io_service &){},
             thread_pool_size)
    {}
    
    server::~server()
    { if(init_state_) finalization_handler_(io_service_); }
    
    void server::listen(const sap_desc_t &sd) {
        const endpoint_t &ep=sd.first;
        // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
        endpoint_resolver<tcp> resolver;
        tcp::endpoint endpoint = resolver.resolve(ep, "", io_service_);
        
        sap_ptr sap(new sap_t(tcp::acceptor(io_service_), sd.second));
        saps_.push_back(sap);
        sap_list_t::reverse_iterator i=saps_.rbegin();
        (*i)->first.open(endpoint.protocol());
        (*i)->first.set_option(tcp::acceptor::reuse_address(true));
        (*i)->first.bind(endpoint);
        (*i)->first.listen();
    }
    
    void server::run() {
        if (!init_state_) {
            // TODO: Log error
            return;
        }
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
                          sap->first.async_accept(socket, yield[ec]);
                          if (!ec)
                              handle_connect(std::move(socket), sap->second);
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
                  async_tcp_stream s(std::move(socket), yield);
                  try {
                      handler(s);
                  } catch (std::exception const& e) {
                      // TODO: Log error
                  } catch(...) {
                      // TODO: Log error
                  }
              });
    }
}   // End of namespace net
