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
#include <memory>
#include <chrono>
#include <utility>
#include <boost/asio/write.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

/**
 * Default stream input and output buffer size
 */
constexpr size_t bf_size=4096;
constexpr std::streamsize pb_size=4;

template<typename Protocol>
struct endpoint_resolver;

template<>
struct endpoint_resolver<boost::asio::ip::tcp> {
    boost::asio::ip::tcp::endpoint resolve(const std::string &host,
                                           const std::string &port,
                                           boost::asio::yield_context yield)
    {
        using namespace boost::asio::ip;
        boost::asio::io_service &ios=yield.handler_.dispatcher_.get_io_service();
        tcp::resolver resolver(ios);
        tcp::resolver::query query(host, port);
        return *resolver.async_resolve(query, yield);
    }
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
    typedef async_stream<StreamDescriptor> this_t;
    
    // Constructors
    
    /**
     * Construct async_stream with existing socket and yield context
     */
    async_stream(StreamDescriptor &&socket, boost::asio::yield_context yield)
    : async_stream(make_streambuf(std::move(socket), yield))
    {}
    
    /**
     * Construct async_stream by connecting to specific endpoint
     *
     * NOTE: Why variadic template arg can only be placed at the end?
     */
    template<typename... Args>
    async_stream(boost::asio::yield_context yield,
                 const Args&... args)
    : async_stream(make_streambuf(yield, args...))
    {}
    
    // Movable
    async_stream(async_stream &&src)
    : std::iostream(src.sbuf_.get())
    , sbuf_(std::move(src.sbuf_))
    {}
    
    // Non-copyable
    async_stream(const async_stream&) = delete;
    async_stream& operator=(const async_stream&) = delete;
    
    /**
     * Close underlying stream device, flushing if necessary
     */
    inline void close() {
        if(sbuf_->is_open()) {
            flush();
        }
        sbuf_->close();
    }
    
    inline bool is_open() const
    { return sbuf_->is_open(); }
    
    inline boost::asio::yield_context yield_context()
    { return sbuf_->yield_; }
    
    inline StreamDescriptor &stream_descriptor()
    { return sbuf_->sd_; }
    
    // TODO: Find a proper way to retrieve strand object
    inline boost::asio::strand &strand()
    { return yield_context().handler_.dispatcher_; }
    
    inline boost::asio::io_service &io_service()
    { return yield_context().handler_.dispatcher_.get_io_service(); }
    
    template <typename Function>
    inline void spawn(BOOST_ASIO_MOVE_ARG(Function) function) {
        boost::asio::spawn(strand(), BOOST_ASIO_MOVE_CAST(Function)(function));
    }
    
    inline int read_timeout() const
    { return sbuf_->read_timeout_; }
    
    inline int write_timeout() const
    { return sbuf_->write_timeout_; }
    
    inline void read_timeout(int timeout)
    { sbuf_->read_timeout_=timeout; }
    
    inline void write_timeout(int timeout)
    { sbuf_->write_timeout_=timeout; }
    
private:
    class async_streambuf : public std::streambuf {
    public:
        typedef std::function<void(boost::system::error_code)> timeout_callback_t;
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
        , read_timer_(io_service())
        , write_timer_(io_service())
        {
            setg(buffer_in_ + pb_size,
                 buffer_in_ + pb_size,
                 buffer_in_ + pb_size);
            setp(buffer_out_,
                 buffer_out_ + bf_size - 1 );
        }
        
        // Movable
        async_streambuf(async_streambuf &&src)
        : sd_(std::move(src.sd_))
        , yield_(std::move(src.yield_))
        , buffer_in_(std::move(src.buffer_in_))
        , buffer_out_(std::move(src.buffer_out_))
        , read_timer_(std::move(src.read_timer_))
        , write_timer_(std::move(src.write_timer_))
        {}
        
        // Non-copyable
        async_streambuf(const async_streambuf&) = delete;
        async_streambuf& operator=(const async_streambuf&) = delete;
        
        ~async_streambuf()
        { close(); }
        
        inline bool is_open() const
        { return sd_.is_open(); }
        
        inline void close() {
            if(sd_.is_open()) {
                boost::system::error_code ec;
                sd_.close(ec);
            }
        }
        
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
        { return nudge_()==0 ? 0 : -1; }
        
        virtual std::streamsize showmanyc() {
            if ( gptr() == egptr() ) {
                // Getting area is empty
                return fetch_();
            }
            return egptr()-gptr();
        }
        
    private:
        typedef boost::asio::steady_timer timer_t;
        
        inline boost::asio::strand &strand()
        { return yield_.handler_.dispatcher_; }
        
        inline boost::asio::io_service &io_service()
        { return yield_.handler_.dispatcher_.get_io_service(); }
        
        inline size_t async_read_some_with_timeout(boost::system::error_code &ec) {
            if (read_timeout_>0) {
                // Setup read timer
                read_timer_.expires_from_now(std::chrono::seconds(read_timeout_));
                read_timer_.async_wait(strand().wrap(read_timeout_callback_));
                size_t ret=sd_.async_read_some(boost::asio::buffer(buffer_in_ + pb_size,
                                                                   bf_size - pb_size),
                                               yield_[ec]);
                if(!ec) read_timer_.cancel(ec);
                return ret;
            } else {
                return sd_.async_read_some(boost::asio::buffer(buffer_in_ + pb_size,
                                                               bf_size - pb_size),
                                           yield_[ec]);
            }
        }
        
        inline void async_write_with_timeout(boost::system::error_code &ec) {
            if (write_timeout_>0) {
                // Setup write timer
                write_timer_.expires_from_now(std::chrono::seconds(write_timeout_));
                write_timer_.async_wait(strand().wrap(write_timeout_callback_));
                boost::asio::async_write(sd_,
                                         boost::asio::const_buffers_1(pbase(),
                                                                      pptr()-pbase()),
                                         yield_[ec]);
                if(!ec) write_timer_.cancel(ec);
            } else {
                boost::asio::async_write(sd_,
                                         boost::asio::const_buffers_1(pbase(),
                                                                      pptr()-pbase()),
                                         yield_[ec]);
            }
        }
        
        inline int_type fetch_() {
            std::streamsize num = std::min(static_cast<std::streamsize>(gptr() - eback()),
                                           pb_size);
            
            std::memmove(buffer_in_ + (pb_size - num),
                         gptr() - num,
                         num);
            
            boost::system::error_code ec;
            std::size_t n = async_read_some_with_timeout(ec);
            if (ec) {
                setg(0, 0, 0);
                return -1;
            }
            setg(buffer_in_ + pb_size - num,
                 buffer_in_ + pb_size,
                 buffer_in_ + pb_size + n);
            return n;
        }
        
        inline int_type nudge_() {
            // Don't flush empty buffer
            if(pptr()<=pbase()) return 0;
            boost::system::error_code ec;
            async_write_with_timeout(ec);
            setp(buffer_out_,
                 buffer_out_ + bf_size - 1);
            return ec ? traits_type::eof() : 0;
        }
        
        StreamDescriptor sd_;
        boost::asio::yield_context yield_;
        char buffer_in_[bf_size];
        char buffer_out_[bf_size];
        int read_timeout_=0;
        int write_timeout_=0;
        timer_t read_timer_;
        timer_t write_timer_;
        timeout_callback_t read_timeout_callback_;
        timeout_callback_t write_timeout_callback_;
        
        friend class async_stream<StreamDescriptor>;
    };
    
    typedef std::shared_ptr<async_streambuf> streambuf_ptr;

    /**
     * Constructor
     *
     * @param sb a shared pointer to the async_streambuf
     */
    async_stream(streambuf_ptr sb)
    : sbuf_(sb)
    , std::iostream(sb.get())
    { setup_callback(); }
    
    static inline streambuf_ptr make_streambuf(StreamDescriptor &&socket,
                                               boost::asio::yield_context yield)
    { return std::make_shared<async_streambuf>(std::move(socket), yield); }
    
    template<typename... Args>
    static inline streambuf_ptr make_streambuf(boost::asio::yield_context yield,
                                               const Args &... args)
    {
        endpoint_resolver<typename StreamDescriptor::protocol_type> resolver;
        StreamDescriptor s(yield.handler_.dispatcher_.get_io_service());
        s.async_connect(resolver.resolve(args..., yield), yield);
        return std::make_shared<async_streambuf>(std::move(s), yield);
    }
    
    inline void setup_callback() {
        // NOTE: shared_ptr<async_streambuf> is needed here as the callback may happen *after* the destruction of async_stream
        // Cancelling in destructor is not reliable as the callback may already be posted in the queue hence not cancelable
        std::weak_ptr<async_streambuf> sbp(sbuf_);
        sbuf_->read_timeout_callback_=[sbp](boost::system::error_code error){
            if (!error && !sbp.expired()) sbp.lock()->close();
        };
        sbuf_->write_timeout_callback_=[sbp](boost::system::error_code error){
            if (!error && !sbp.expired()) sbp.lock()->close();
        };
    }
    
    streambuf_ptr sbuf_;
};

typedef async_stream<boost::asio::ip::tcp::socket> async_tcp_stream;

#endif  /* defined(async_stream_h_included) */
