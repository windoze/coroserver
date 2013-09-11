//
//  session.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/coroutine/coroutine.hpp>

#ifndef session_h_included
#define session_h_included

class session : private boost::noncopyable
{
public:
    typedef boost::tuple<boost::system::error_code, std::size_t> tuple_t;
    typedef boost::coroutines::coroutine<void(boost::system::error_code, std::size_t)> coro_t;
    
    session(boost::asio::io_service &io_service);
    
    void start();

    inline boost::asio::ip::tcp::socket &socket()
    { return socket_; }
    
private:
    void go(boost::asio::yield_context yield);
    void destroy();
    
    coro_t coro_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand strand_;
};

#endif /* defined(session_h_included) */
