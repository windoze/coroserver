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

#ifndef server_h_included
#define server_h_included

class session;

class server {
public:
    server(const std::string &address,
           const std::string &port,
           std::size_t thread_pool_size);
    server(const server&) = delete;
    server& operator=(const server&) = delete;
    void run();

private:
    void start();
    void stop();

    std::size_t thread_pool_size_;
    boost::asio::io_service io_service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* defined(server_h_included) */
