//
//  async_stream.h
//  coroserver
//
//  Created by Windoze on 13-2-24.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef async_stream_h_included
#define async_stream_h_included

#include <streambuf>
#include <iostream>
#include <boost/asio/write.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>

/**
 * Default stream input and output buffer size
 */
constexpr size_t bf_size=4096;
constexpr std::streamsize pb_size=4;

/**
 * Input/output socket streambuf with coroutine support
 * Yields on buffer overflow or underflow, and gets resumed on io completion
 *
 * @param StreamDescriptor an asychronized stream descriptor, i.e. boost::asio::ip::tcp::socket, or boost::asio::stream_descriptor with native handle
 */
template<typename StreamDescriptor>
class async_streambuf : public std::streambuf {
public:
    /**
     * Constructor
     *
     * @param sd underlying stream device, such as socket or pipe
     * @param yield the yield_context used by coroutines
     */
    async_streambuf(StreamDescriptor &&sd,
                    boost::asio::yield_context yield)
    : sd_(std::move(sd))
    , yield_(yield)
    , buffer_in_()
    , buffer_out_()
    {
        setg(buffer_in_ + pb_size,
             buffer_in_ + pb_size,
             buffer_in_ + pb_size);
        setp(buffer_out_,
             buffer_out_ + bf_size - 1 );
    }

    // Non-copyable
    async_streambuf(const async_streambuf&) = delete;
    async_streambuf& operator=(const async_streambuf&) = delete;
    
    inline bool is_open() const
    { return sd_.is_open(); }
    
    inline void close() {
        if(sd_.is_open())
            sd_.close();
    }
    
    inline boost::asio::yield_context yield_context()
    { return yield_; }
    
    inline StreamDescriptor &stream_descriptor()
    { return sd_; }
    
protected:
    virtual int_type overflow(int_type c) {
        if ( c != traits_type::eof() ) {
            *pptr() = c;
            pbump(1);
            if (nudge_())
                return c;
        } else {
            c = 0;
        }

        return traits_type::eof();
    }

    virtual int_type underflow() {
        if ( gptr() < egptr() )
            return traits_type::to_int_type( *gptr() );
        
        if ( 0 > fetch_() )
            return traits_type::eof();
        else
            return traits_type::to_int_type( *gptr() );
    }

    virtual int sync()
    { return nudge_() ? 0 : -1; }
    
    virtual std::streamsize showmanyc() {
        if ( gptr() == egptr() ) {
            // Getting area is empty
            return fetch_();
        }
        return egptr()-gptr();
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
        // Don't flush empty buffer
        std::ptrdiff_t n=pptr()-pbase();
        if(n<=0) return 0;
        boost::system::error_code ec;
        boost::asio::async_write(sd_,
                                 boost::asio::const_buffers_1(pbase(),
                                                              n),
                                 yield_[ec]);
        setp(buffer_out_,
             buffer_out_ + bf_size - 1);
        return ec ? traits_type::eof() : 0;
    }
    
    StreamDescriptor sd_;
    boost::asio::yield_context yield_;
    char buffer_in_[bf_size];
    char buffer_out_[bf_size];
};

/*
 * std::iostream with coroutine support
 * Yields on blocking of reading or writing, and gets resumed on io completion
 *
 * @param StreamDescriptor an asychronized stream descriptor, i.e. boost::asio::ip::tcp::socket, or boost::asio::stream_descriptor with native handle
 */
template<typename StreamDescriptor>
class async_stream : public std::iostream {
public:
    /**
     * Constructor
     *
     * @param sd underlying stream device, such as socket or pipe
     * @param yield the yield_context used by coroutines
     */
    async_stream(StreamDescriptor &&sd, boost::asio::yield_context yield)
    : sbuf_(std::move(sd), yield)
    , std::iostream(&sbuf_)
    {}
    
    /**
     * Destructor
     */
    ~async_stream()
    { close(); }
    
    /**
     * Close underlying stream device, flushing if necessary
     */
    inline void close() {
        if(sbuf_.is_open()) {
            flush();
        }
        sbuf_.close();
    }
    
    inline bool is_open() const
    { return sbuf_.is_open(); }
    
    inline boost::asio::yield_context yield_context()
    { return sbuf_.yield_context(); }

    inline StreamDescriptor &stream_descriptor()
    { return sbuf_.stream_descriptor(); }

private:
    async_streambuf<StreamDescriptor> sbuf_;
};

typedef async_stream<boost::asio::ip::tcp::socket> async_tcp_stream;

#endif  /* defined(async_stream_h_included) */
