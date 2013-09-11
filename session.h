//
//  session.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <memory>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#ifndef session_h_included
#define session_h_included

class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(boost::asio::ip::tcp::socket socket);
    session(const session&) = delete;
    session& operator=(const session&) = delete;
    
    void start();

    inline boost::asio::ip::tcp::socket &socket()
    { return socket_; }
    
private:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand strand_;
};

#endif /* defined(session_h_included) */
