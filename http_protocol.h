//
//  http_protocol.h
//  coroserver
//
//  Created by Windoze on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef __coroserver__http_protocol__
#define __coroserver__http_protocol__

#include <string>
#include <vector>
#include <iostream>
#include "async_stream.h"

namespace http {
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
         * Set to true means the response has been processed by the handler, no further actions needed
         */
        bool raw_;
        
        /**
         * HTTP status code
         */
        int code_;
        
        /**
         * HTTP Headers
         */
        headers_t headers_;
        
        /**
         * Should the body be compressed
         */
        bool compress_;

        /**
         * Output stream for response body, only used when raw_ is false
         */
        std::ostream &body_stream_;

        /**
         * Output stream for raw response, handler needs to output status line, headers, body, etc by itself.
         */
        std::ostream &raw_stream_;
        
        response_t(std::ostream &body_stream, std::ostream &raw_stream)
        : raw_(false)
        , code_(200)
        , body_stream_(body_stream)
        , raw_stream_(raw_stream)
        {}
    };
    
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
    bool protocol_handler(async_tcp_stream& s);
    
}   // End of namespace http

#endif /* defined(__coroserver__http_protocol__) */
