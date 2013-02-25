//
//  server.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>

#ifndef server_h_included
#define server_h_included

class session;

class server : private boost::noncopyable {
public:
    server(const std::string &address,
           const std::string &port,
           std::size_t thread_pool_size);
    void run();

private:
    void start_accept();
    void handle_accept(session *new_session,
                       const boost::system::error_code& e);
    void handle_stop();

    std::size_t thread_pool_size_;
    boost::asio::io_service io_service_;
    boost::asio::signal_set signals_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

#endif /* defined(server_h_included) */
