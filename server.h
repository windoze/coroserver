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
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include "async_stream.h"

typedef std::pair<std::string, std::string> endpoint_t;
typedef std::vector<endpoint_t> endpoint_list_t;

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
    server(std::function<bool(async_tcp_stream_ptr)> protocol_processor,
           const endpoint_list_t &endpoints,
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
    void listen(const endpoint_list_t &epl);
    void listen(const endpoint_t &ep);
    void close();
    void run();
    void handle_connect(boost::asio::ip::tcp::socket &&socket);

    std::function<bool(async_tcp_stream_ptr)> protocol_processor_;
    std::size_t thread_pool_size_;
    boost::asio::io_service io_service_;
    boost::asio::signal_set signals_;
    typedef std::vector<boost::asio::ip::tcp::acceptor> acceptor_list_t;
    acceptor_list_t acceptors_;
};

#endif /* defined(server_h_included) */
