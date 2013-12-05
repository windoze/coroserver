//
//  server.h
//  coroserver
//
//  Created by Windoze on 13-2-24.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef server_h_included
#define server_h_included

#include <functional>
#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include "async_stream.h"

typedef std::pair<std::string, std::string> endpoint_t;
typedef std::function<void(async_tcp_stream&)> protocol_handler_t;
typedef std::pair<protocol_handler_t, endpoint_t> sap_desc_t;
typedef std::vector<sap_desc_t> sap_desc_list_t;

/**
 * Stream-oriented socket server
 */
class server {
public:
    /**
     * Constructor
     *
     * @param protocol_processor the functor that processes the socket stream
     * @param endpoints listening addresses and ports
     * @param thread_pool_size number of threads that run simultaneously to process client connections
     */
    server(const sap_desc_list_t &sap_desc_list,
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
    void listen(const sap_desc_t &sd);
    void open();
    void close();
    void run();
    void handle_connect(boost::asio::ip::tcp::socket &&socket, const protocol_handler_t &handler);

    std::size_t thread_pool_size_;
    boost::asio::io_service io_service_;
    boost::asio::signal_set signals_;
    typedef std::pair<protocol_handler_t, boost::asio::ip::tcp::acceptor> sap_t;
    typedef std::shared_ptr<sap_t> sap_ptr;
    typedef std::vector<sap_ptr> sap_list_t;
    sap_list_t saps_;
};

#endif /* defined(server_h_included) */
