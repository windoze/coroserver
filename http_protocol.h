//
//  http_protocol.h
//  coroserver
//
//  Created by Windoze on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef __coroserver__http_protocol__
#define __coroserver__http_protocol__

#define HTTP_SERVER_NAME "coroserver"
#define HTTP_SERVER_VERSION "0.1"

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <boost/interprocess/streams/vectorstream.hpp>
#include "async_stream.h"

namespace http {
    extern const char *server_name;
    
    enum method {
        DELETE,
        GET,
        HEAD,
        POST,
        PUT,
        /* pathological */
        CONNECT,
        OPTIONS,
        TRACE,
        /* webdav */
        COPY,
        LOCK,
        MKCOL,
        MOVE,
        PROPFIND,
        PROPPATCH,
        SEARCH,
        UNLOCK,
        /* subversion */
        REPORT,
        MKACTIVITY,
        CHECKOUT,
        MERGE,
        /* upnp */
        MSEARCH,
        NOTIFY,
        SUBSCRIBE,
        UNSUBSCRIBE,
        /* RFC-5789 */
        PATCH,
        PURGE,
    };
    
    enum status_code {
        CONTINUE                        =100,
        SWITCHING_PROTOCOLS             =101,
        OK                              =200,
        CREATED                         =201,
        ACCEPTED                        =202,
        NON_AUTHORITATIVE_INFORMATION   =203,
        NO_CONTENT                      =204,
        RESET_CONTENT                   =205,
        PARTIAL_CONTENT                 =206,
        MULTIPLE_CHOICES                =300,
        MOVED_PERMANENTLY               =301,
        FOUND                           =302,
        SEE_OTHER                       =303,
        NOT_MODIFIED                    =304,
        USE_PROXY                       =305,
        //UNUSED                        =306,
        TEMPORARY_REDIRECT              =307,
        BAD_REQUEST                     =400,
        UNAUTHORIZED                    =401,
        PAYMENT_REQUIRED                =402,
        FORBIDDEN                       =403,
        NOT_FOUND                       =404,
        METHOD_NOT_ALLOWED              =405,
        NOT_ACCEPTABLE                  =406,
        PROXY_AUTHENTICATION_REQUIRED   =407,
        REQUEST_TIMEOUT                 =408,
        CONFLICT                        =409,
        GONE                            =410,
        LENGTH_REQUIRED                 =411,
        PRECONDITION_FAILED             =412,
        REQUEST_ENTITY_TOO_LARGE        =413,
        REQUEST_URI_TOO_LONG            =414,
        UNSUPPORTED_MEDIA_TYPE          =415,
        REQUESTED_RANGE_NOT_SATISFIABLE =416,
        EXPECTATION_FAILED              =417,
        INTERNAL_SERVER_ERROR           =500,
        NOT_IMPLEMENTED                 =501,
        BAD_GATEWAY                     =502,
        SERVICE_UNAVAILABLE             =503,
        GATEWAY_TIMEOUT                 =504,
        HTTP_VERSION_NOT_SUPPORTED      =505,
    };
    
    typedef std::pair<std::string, std::string> header_t;
    typedef std::vector<header_t> headers_t;
    
    struct request_t {
        short http_major_;
        short http_minor_;
        method method_;
        std::string schema_;
        std::string host_;
        int port_;
        std::string path_;
        std::string query_;
        std::string user_info_;
        headers_t headers_;
        std::string body_;
        bool keep_alive_;
        bool closed_;
    };
    
    /**
     * Parse HTTP request
     *
     * @param is the socket stream
     * @param req the HTTP request struct to be filled
     */
    bool parse(std::istream &is, request_t &req);
    
    struct response_t {
        /**
         * HTTP status code
         */
        status_code code_=OK;
        
        /**
         * HTTP Headers
         */
        headers_t headers_;
        
        /**
         * The response body
         */
        std::string body_;

        /**
         * Should the response be compressed
         */
        bool compress_;
    };
    
    /**
     * Parse HTTP request
     *
     * @param is the socket stream
     * @param req the HTTP request struct to be filled
     */
    bool parse(std::istream &is, response_t &resp);
    
    struct session_t {
        typedef boost::interprocess::basic_ovectorstream<std::string> body_stream_t;
        
        session_t(async_tcp_stream_ptr raw_stream)
        : raw_stream_(raw_stream)
        {}

        inline bool parse_request()
        { return parse(raw_stream(), request_); }
        
        inline bool parse_response()
        { return parse(raw_stream(), response_); }
            
        inline bool closed() const
        { return request_.closed_; }
        
        inline bool raw() const
        { return raw_; }
        
        inline void raw(bool r)
        { raw_=r; }
        
        inline bool keep_alive() const
        { return request_.keep_alive_; }
        
        inline body_stream_t &body_stream()
        { return body_stream_; }
        
        inline async_tcp_stream &raw_stream()
        { return *raw_stream_; }
        
        inline boost::asio::strand &strand()
        { return raw_stream().strand(); }
        
        inline boost::asio::io_service &io_service()
        { return raw_stream().strand().get_io_service(); }
        
        inline boost::asio::yield_context yield_context()
        { return raw_stream().yield_context(); }
        
        template <typename Function>
        void spawn(BOOST_ASIO_MOVE_ARG(Function) function) {
            boost::asio::spawn(strand(), BOOST_ASIO_MOVE_CAST(Function)(function));
        }
        
        // private:
        /**
         * HTTP request
         */
        request_t request_;
        
        /**
         * HTTP response
         */
        response_t response_;
        
        /**
         * Set to true means the response has been processed by the handler, no further actions needed
         */
        bool raw_=false;
        
        /**
         * Set to true means the process has been deferred
         */
        bool deferred_=false;

        /**
         * Output stream for response body, only used when raw_ is false
         */
        body_stream_t body_stream_;
        
        /**
         * Output stream for raw response, handler needs to output status line, headers, body, etc by itself.
         */
        async_tcp_stream_ptr raw_stream_;
    };
    
    typedef std::shared_ptr<session_t> session_ptr;
    
    // TODO:
    /**
     * Parse HTTP response
     *
     * @param is the socket stream
     * @param req the HTTP response struct to be filled
     */
    // bool parse(std::istream &is, response_t &resp);

    /**
     * Handle HTTP protocol handler
     *
     * @param s the socket stream
     * @param yield the yield_context, can be used to create new connections
     */
    bool protocol_handler(async_tcp_stream_ptr s);
    
}   // End of namespace http

#endif /* defined(__coroserver__http_protocol__) */
