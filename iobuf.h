//
//  iobuf.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <streambuf>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

#ifndef iobuf_h_included
#define iobuf_h_included

constexpr size_t bf_size=4096;

/*
 * Input socket streambuf with coroutine support
 * Yields on buffer underflow, and gets resumed on io completion
 */
class async_inbuf : public std::streambuf {
public:
    async_inbuf(boost::asio::ip::tcp::socket &s,
                boost::asio::yield_context yield);
    async_inbuf(const async_inbuf&) = delete;
    async_inbuf& operator=(const async_inbuf&) = delete;

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
class async_outbuf : public std::streambuf {
public:
    async_outbuf(boost::asio::ip::tcp::socket &s,
                 boost::asio::yield_context yield);
    async_outbuf(const async_outbuf&) = delete;
    async_outbuf& operator=(const async_outbuf&) = delete;
    
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
