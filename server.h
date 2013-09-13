//
//  server.h
//  coroserver
//
//  Created by Windoze on 13-2-24.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef server_h_included
#define server_h_included

#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include "async_stream.h"

/**
 * Stream-oriented socket server
 */
class server {
public:
    /**
     * Constructor
     *
     * @param simple_protocol_processor the functor that processes the stream, a yield_context will be passed to the function along with socket stream
     * @param address listening address
     * @param port listening port
     * @param thread_pool_size number of threads that run simultaneously to process client connections
     */
    server(std::function<bool(async_tcp_stream&)> protocol_processor,
           const std::string &address,
           const std::string &port,
           std::size_t thread_pool_size);
    
    // Non-copyable
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    
    /**
     * Start server
     */
    inline void operator()()
    { run(); }

private:
    void open();
    void close();
    void run();
    void handle_connect(boost::asio::ip::tcp::socket &&socket);

    std::function<bool(async_tcp_stream&)> protocol_processor_;
    std::size_t thread_pool_size_;
    boost::asio::io_service io_service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* defined(server_h_included) */
