//
//  server.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>

#ifndef server_h_included
#define server_h_included

class server {
public:
    server(std::function<bool(std::iostream&)> simple_protocol_processor,
           const std::string &address,
           const std::string &port,
           std::size_t thread_pool_size);
    server(std::function<bool(std::iostream&, boost::asio::yield_context)> protocol_processor,
           const std::string &address,
           const std::string &port,
           std::size_t thread_pool_size);
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    void run();

private:
    void init(const std::string &address,
              const std::string &port);
    void start();
    void stop();

    inline bool simple_processor_wrapper(std::iostream& s, boost::asio::yield_context) {
        return proc_simple_(s);
    }

    std::function<bool(std::iostream&, boost::asio::yield_context)> protocol_processor_;
    std::function<bool(std::iostream&)> proc_simple_;
    std::size_t thread_pool_size_;
    boost::asio::io_service io_service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* defined(server_h_included) */
