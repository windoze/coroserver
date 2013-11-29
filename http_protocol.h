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
    
    typedef std::shared_ptr<request_t> request_ptr;
    
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
        status_code code_;
        
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
        async_tcp_stream_ptr raw_stream_;
        
        response_t(std::ostream &body_stream, async_tcp_stream_ptr raw_stream)
        : raw_(false)
        , code_(OK)
        , body_stream_(body_stream)
        , raw_stream_(raw_stream)
        {}
    };
    
    typedef std::shared_ptr<response_t> response_ptr;
    
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
