//
//  iobuf.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <streambuf>
#include <boost/asio/write.hpp>
#include <boost/asio/spawn.hpp>

#ifndef iobuf_h_included
#define iobuf_h_included

constexpr size_t bf_size=4096;
constexpr std::streamsize pb_size=4;

/*
 * Input/output socket streambuf with coroutine support
 * Yields on buffer overflow or underflow, and gets resumed on io completion
 */

template<typename StreamDescriptor>
class async_streambuf : public std::streambuf {
public:
    async_streambuf(StreamDescriptor &sd,
                    boost::asio::yield_context yield)
    : sd_(sd)
    , yield_(yield)
    , buffer_in_()
    , buffer_out_()
    {
        setg(buffer_in_ + pb_size,
             buffer_in_ + pb_size,
             buffer_in_ + pb_size);
    }

    // Non-copyable
    async_streambuf(const async_streambuf&) = delete;
    async_streambuf& operator=(const async_streambuf&) = delete;
    
protected:
    virtual int_type overflow(int_type c = traits_type::eof()) {
        if ( pbase() == NULL ) {
            // save one char for next overflow:
            setp( buffer_out_, buffer_out_ + bf_size - 1 );
            if ( c != traits_type::eof() ) {
                c = sputc( c );
            } else {
                c = 0;
            }
        } else {
            c = nudge_();
            setp(buffer_out_,
                 buffer_out_ + bf_size - 1);
        }
        return c;
    }

    virtual int_type underflow() {
        if ( gptr() < egptr() )
            return traits_type::to_int_type( *gptr() );
        
        if ( 0 > fetch_() )
            return traits_type::eof();
        else
            return traits_type::to_int_type( *gptr() );
    }

    virtual int sync() {
        int_type n = nudge_();
        setp(buffer_out_,
             buffer_out_ + bf_size - 1);
        if (n==traits_type::eof()) {
            return -1;
        }
        return 0;
    }
    
private:
    int_type fetch_() {
        std::streamsize num = std::min(static_cast<std::streamsize>(gptr() - eback()),
                                       pb_size);
        
        std::memmove(buffer_in_ + (pb_size - num),
                     gptr() - num,
                     num);
        
        boost::system::error_code ec;
        std::size_t n = sd_.async_read_some(boost::asio::buffer(buffer_in_ + pb_size,
                                                                bf_size - pb_size),
                                            yield_[ec]);
        if (ec) {
            setg(0, 0, 0);
            return -1;
        }
        setg(buffer_in_ + pb_size - num,
             buffer_in_ + pb_size,
             buffer_in_ + pb_size + n);
        return n;
    }

    int_type nudge_() {
        boost::system::error_code ec;
        boost::asio::async_write(sd_,
                                 boost::asio::const_buffers_1(pbase(),
                                                              pptr() - pbase()),
                                 yield_[ec]);
        return ec ? traits_type::eof() : 0;
    }
    
    StreamDescriptor &sd_;
    boost::asio::yield_context yield_;
    char buffer_in_[bf_size];
    char buffer_out_[bf_size];
};

#endif  /* defined(iobuf_h_included) */
