//
//  iobuf.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include "iobuf.h"

const std::streamsize inbuf::pb_size=16;

inbuf::inbuf(boost::asio::ip::tcp::socket &s,
             boost::asio::yield_context yield)
: s_(s)
, yield_(yield)
, buffer_()
{
    setg(buffer_ + pb_size, buffer_ + pb_size, buffer_ + pb_size);
}

int inbuf::underflow() {
    if ( gptr() < egptr() )
        return traits_type::to_int_type( *gptr() );
    
    if ( 0 > fetch_() )
        return traits_type::eof();
    else
        return traits_type::to_int_type( *gptr() );
}

int inbuf::fetch_() {
    std::streamsize num = std::min(static_cast< std::streamsize >( gptr() - eback() ), pb_size);
    
    std::memmove(buffer_ + (pb_size - num),
                 gptr() - num, num);
    
    std::size_t n = s_.async_read_some( boost::asio::buffer(buffer_ + pb_size, bf_size - pb_size),
                       yield_ );
    setg(buffer_ + pb_size - num, buffer_ + pb_size, buffer_ + pb_size + n);
    return n;
}

outbuf::outbuf(boost::asio::ip::tcp::socket &s,
               boost::asio::yield_context yield)
: s_(s)
, yield_(yield)
{}

outbuf::int_type outbuf::overflow(outbuf::int_type c) {
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
        setp( buffer_, buffer_ + bf_size - 1 );
    }
    return c;
}

int outbuf::sync() {
    int_type n = nudge_();
    setp( buffer_, buffer_ + bf_size - 1 );
    if (n==traits_type::eof()) {
        return -1;
    }
    return 0;
}

outbuf::int_type outbuf::nudge_() {
    boost::asio::async_write(s_,
                             boost::asio::const_buffers_1(pbase(), pptr() - pbase()),
                             yield_ );
    return 0;
}
