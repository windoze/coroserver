//
//  iobuf.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <boost/asio/write.hpp>
#include "iobuf.h"

const std::streamsize async_inbuf::pb_size=4;

async_inbuf::async_inbuf(boost::asio::ip::tcp::socket &s,
                         boost::asio::yield_context yield)
: s_(s)
, yield_(yield)
, buffer_()
{
    setg(buffer_ + pb_size,
         buffer_ + pb_size,
         buffer_ + pb_size);
}

int async_inbuf::underflow() {
    if ( gptr() < egptr() )
        return traits_type::to_int_type( *gptr() );
    
    if ( 0 > fetch_() )
        return traits_type::eof();
    else
        return traits_type::to_int_type( *gptr() );
}

int async_inbuf::fetch_() {
    std::streamsize num = std::min(static_cast<std::streamsize>(gptr() - eback()),
                                   pb_size);
    
    std::memmove(buffer_ + (pb_size - num),
                 gptr() - num,
                 num);
    
    boost::system::error_code ec;
    std::size_t n = s_.async_read_some(boost::asio::buffer(buffer_ + pb_size,
                                                           bf_size - pb_size),
                                       yield_[ec]);
    if (ec) {
        setg(0, 0, 0);
        return -1;
    }
    setg(buffer_ + pb_size - num,
         buffer_ + pb_size,
         buffer_ + pb_size + n);
    return n;
}

async_outbuf::async_outbuf(boost::asio::ip::tcp::socket &s,
                           boost::asio::yield_context yield)
: s_(s)
, yield_(yield)
{}

async_outbuf::int_type async_outbuf::overflow(async_outbuf::int_type c) {
    if ( pbase() == NULL ) {
        // save one char for next overflow:
        setp( buffer_, buffer_ + bf_size - 1 );
        if ( c != traits_type::eof() ) {
            c = sputc( c );
        } else {
            c = 0;
        }
    } else {
        c = nudge_();
        setp(buffer_,
             buffer_ + bf_size - 1);
    }
    return c;
}

int async_outbuf::sync() {
    int_type n = nudge_();
    setp(buffer_,
         buffer_ + bf_size - 1);
    if (n==traits_type::eof()) {
        return -1;
    }
    return 0;
}

async_outbuf::int_type async_outbuf::nudge_() {
    boost::system::error_code ec;
    boost::asio::async_write(s_,
                             boost::asio::const_buffers_1(pbase(),
                                                          pptr() - pbase()),
                             yield_[ec]);
    return ec ? traits_type::eof() : 0;
}
