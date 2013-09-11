//
//  iobuf.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <streambuf>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/coroutine/coroutine.hpp>

#ifndef iobuf_h_included
#define iobuf_h_included

typedef boost::tuple<boost::system::error_code, std::size_t> tuple_t;
typedef boost::coroutines::coroutine<void(boost::system::error_code, std::size_t)> coro_t;
constexpr size_t bf_size=4096;

/*
 * Input socket streambuf with coroutine support
 * Yields on buffer underflow, and gets resumed on io completion
 */
class inbuf : public std::streambuf, private boost::noncopyable {
public:
    inbuf(boost::asio::ip::tcp::socket &s,
          boost::asio::yield_context yield);

protected:
    virtual int underflow() override;
    
private:
    int fetch_();
    
    static const std::streamsize pb_size;
    
    boost::asio::ip::tcp::socket &s_;
    boost::asio::yield_context yield_;
    char buffer_[bf_size];
};

/*
 * Output socket streambuf with coroutine support
 * Yields on buffer overflow or syncing, gets resumed on io completion
 */
class outbuf : public std::streambuf, private boost::noncopyable {
public:
    outbuf(boost::asio::ip::tcp::socket &s,
           boost::asio::yield_context yield);
    
protected:
    virtual int_type overflow(int_type c = traits_type::eof()) override;
    virtual int sync() override;
    
private:
    int_type nudge_();
    
    boost::asio::ip::tcp::socket &s_;
    boost::asio::yield_context yield_;
    char buffer_[bf_size];
};

#endif  /* defined(iobuf_h_included) */
