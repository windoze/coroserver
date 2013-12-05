//
//  http_protocol.cpp
//  coroserver
//
//  Created by Windoze on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <map>
#include <iostream>
#include <boost/interprocess/streams/vectorstream.hpp>
#include "http-parser/http_parser.h"
#include "http_protocol.h"

namespace http {
    const char *server_name=HTTP_SERVER_NAME "/" HTTP_SERVER_VERSION;
    typedef std::function<bool(session_t &)> parse_callback_t;
    
    namespace details {
        const std::map<status_code, std::string> status_code_msg_map={
            {CONTINUE                       , "Continue"},
            {SWITCHING_PROTOCOLS            , "Switching Protocols"},
            {OK                             , "OK"},
            {CREATED                        , "Created"},
            {ACCEPTED                       , "Accepted"},
            {NON_AUTHORITATIVE_INFORMATION  , "Non-Authoritative Information"},
            {NO_CONTENT                     , "No Content"},
            {RESET_CONTENT                  , "Reset Content"},
            {PARTIAL_CONTENT                , "Partial Content"},
            {MULTIPLE_CHOICES               , "Multiple Choices"},
            {MOVED_PERMANENTLY              , "Moved Permanently"},
            {FOUND                          , "Found"},
            {SEE_OTHER                      , "See Other"},
            {NOT_MODIFIED                   , "Not Modified"},
            {USE_PROXY                      , "Use Proxy"},
            {TEMPORARY_REDIRECT             , "Temporary Redirect"},
            {BAD_REQUEST                    , "Bad Request"},
            {UNAUTHORIZED                   , "Unauthorized"},
            {PAYMENT_REQUIRED               , "Payment Required"},
            {FORBIDDEN                      , "Forbidden"},
            {NOT_FOUND                      , "Not Found"},
            {METHOD_NOT_ALLOWED             , "Method Not Allowed"},
            {NOT_ACCEPTABLE                 , "Not Acceptable"},
            {PROXY_AUTHENTICATION_REQUIRED  , "Proxy Authentication Required"},
            {REQUEST_TIMEOUT                , "Request Timeout"},
            {CONFLICT                       , "Conflict"},
            {GONE                           , "Gone"},
            {LENGTH_REQUIRED                , "Length Required"},
            {PRECONDITION_FAILED            , "Precondition Failed"},
            {REQUEST_ENTITY_TOO_LARGE       , "Request Entity Too Large"},
            {REQUEST_URI_TOO_LONG           , "Request-URI Too Long"},
            {UNSUPPORTED_MEDIA_TYPE         , "Unsupported Media Type"},
            {REQUESTED_RANGE_NOT_SATISFIABLE, "Requested Range Not Satisfiable"},
            {EXPECTATION_FAILED             , "Expectation Failed"},
            {INTERNAL_SERVER_ERROR          , "Internal Server Error"},
            {NOT_IMPLEMENTED                , "Not Implemented"},
            {BAD_GATEWAY                    , "Bad Gateway"},
            {SERVICE_UNAVAILABLE            , "Service Unavailable"},
            {GATEWAY_TIMEOUT                , "Gateway Timeout"},
            {HTTP_VERSION_NOT_SUPPORTED     , "HTTP Version Not Supported"},
        };
        
        namespace request {
            struct parser {
                enum parser_state{
                    none,
                    start,
                    url,
                    field,
                    value,
                    body,
                    end
                };
                
                request_t &req() { return session_.request_; }
                
                int on_message_begin() {
                    req().clear();
                    url_.clear();
                    state_=start;
                    return 0;
                }
                int on_url(const char *at, size_t length) {
                    if(state_==url)
                        url_.append(at, length);
                    else {
                        url_.reserve(1024);
                        url_.assign(at, length);
                    }
                    state_=url;
                    return 0;
                }
                int on_status_complete() {
                    return 0;
                }
                int on_header_field(const char *at, size_t length) {
                    if (state_==field) {
                        req().headers_.rbegin()->first.append(at, length);
                    } else {
                        req().headers_.push_back(header_t());
                        req().headers_.rbegin()->first.reserve(256);
                        req().headers_.rbegin()->first.assign(at, length);
                    }
                    state_=field;
                    return 0;
                }
                int on_header_value(const char *at, size_t length) {
                    if (state_==value)
                        req().headers_.rbegin()->second.append(at, length);
                    else {
                        req().headers_.rbegin()->second.reserve(256);
                        req().headers_.rbegin()->second.assign(at, length);
                    }
                    state_=value;
                    return 0;
                }
                int on_headers_complete() {
                    return 0;
                }
                int on_body(const char *at, size_t length) {
                    if (state_==body)
                        req().body_.append(at, length);
                    else {
                        req().body_.reserve(1024);
                        req().body_.assign(at, length);
                    }
                    state_=body;
                    return 0;
                }
                int on_message_complete() {
                    state_=end;
                    req().method_=(method)(parser_.method);
                    req().http_major_=parser_.http_major;
                    req().http_minor_=parser_.http_minor;
                    http_parser_url u;
                    http_parser_parse_url(url_.c_str(),
                                          url_.size(),
                                          req().method_==CONNECT,
                                          &u);
                    // Components for proxy requests
                    // NOTE: Schema, user info, host, and port may only exist in proxy requests
                    if(u.field_set & 1 << UF_SCHEMA) {
                        req().schema_.assign(url_.begin()+u.field_data[UF_SCHEMA].off,
                                             url_.begin()+u.field_data[UF_SCHEMA].off+u.field_data[UF_SCHEMA].len);
                    }
                    if(u.field_set & 1 << UF_USERINFO) {
                        req().user_info_.assign(url_.begin()+u.field_data[UF_USERINFO].off,
                                                url_.begin()+u.field_data[UF_USERINFO].off+u.field_data[UF_USERINFO].len);
                    }
                    if(u.field_set & 1 << UF_HOST) {
                        req().host_.assign(url_.begin()+u.field_data[UF_HOST].off,
                                           url_.begin()+u.field_data[UF_HOST].off+u.field_data[UF_HOST].len);
                    }
                    if(u.field_set & 1 << UF_PORT) {
                        req().port_=u.port;
                    } else {
                        req().port_=0;
                    }
                    // Common components
                    if(u.field_set & 1 << UF_PATH) {
                        req().path_.assign(url_.begin()+u.field_data[UF_PATH].off,
                                           url_.begin()+u.field_data[UF_PATH].off+u.field_data[UF_PATH].len);
                    }
                    if(u.field_set & 1 << UF_QUERY) {
                        req().query_.assign(url_.begin()+u.field_data[UF_QUERY].off,
                                            url_.begin()+u.field_data[UF_QUERY].off+u.field_data[UF_QUERY].len);
                    }
                    req().keep_alive_=http_should_keep_alive(&parser_);
                    return (should_continue_=cb_(session_)) ? 0 : -1;
                }
                
                http_parser parser_;
                std::string url_;
                session_t &session_;
                parse_callback_t &cb_;
                parser_state state_;
                bool should_continue_;
                
                parser(session_t &session, parse_callback_t &cb);
                bool parse();
            };
            
            static int on_message_begin(http_parser*p) {
                return reinterpret_cast<parser *>(p->data)->on_message_begin();
            }
            static int on_url(http_parser*p, const char *at, size_t length) {
                return reinterpret_cast<parser *>(p->data)->on_url(at, length);
            }
            static int on_status_complete(http_parser*p) {
                return reinterpret_cast<parser *>(p->data)->on_status_complete();
            }
            static int on_header_field(http_parser*p, const char *at, size_t length) {
                return reinterpret_cast<parser *>(p->data)->on_header_field(at, length);
            }
            static int on_header_value(http_parser*p, const char *at, size_t length) {
                return reinterpret_cast<parser *>(p->data)->on_header_value(at, length);
            }
            static int on_headers_complete(http_parser*p) {
                return reinterpret_cast<parser *>(p->data)->on_headers_complete();
            }
            static int on_body(http_parser*p, const char *at, size_t length) {
                return reinterpret_cast<parser *>(p->data)->on_body(at, length);
            }
            static int on_message_complete(http_parser*p) {
                return reinterpret_cast<parser *>(p->data)->on_message_complete();
            }
            
            static constexpr http_parser_settings settings_={
                &on_message_begin,
                &on_url,
                &on_status_complete,
                &on_header_field,
                &on_header_value,
                &on_headers_complete,
                &on_body,
                &on_message_complete,
            };
            
            parser::parser(session_t &session, parse_callback_t &cb)
            : session_(session)
            , cb_(cb)
            {
                parser_.data=reinterpret_cast<void*>(this);
                http_parser_init(&parser_, HTTP_REQUEST);
            }
            
            bool parser::parse() {
                should_continue_=true;
                state_=none;
                constexpr int buf_size=1024;
                char buf[buf_size];
                int recved=0;
                int nparsed=0;
                while (session_.raw_stream()) {
                    // Read some data
                    recved = session_.raw_stream().readsome(buf, buf_size);
                    if (recved<=0) {
                        // Connection closed
                        return true;
                    }
                    nparsed=http_parser_execute(&parser_, &settings_, buf, recved);
                    if (!should_continue_) {
                        break;
                    }
                    if (nparsed!=recved) {
                        // Parse error
                        return false;
                    }
                }
                return true;
            }
        }   // End of namespace request
        namespace response {
            // TODO:
        }   // End of namespace response
    }   // End of namespace details
    
    void request_t::clear() {
        http_major_=0;
        http_minor_=0;
        method_=GET;
        schema_.clear();
        user_info_.clear();
        host_.clear();
        port_=0;
        path_.clear();
        query_.clear();
        headers_.clear();
        keep_alive_=false;
        body_.clear();
        std::string empty;
        body_stream_.swap_vector(empty);
    }
    
    void response_t::clear() {
        code_=OK;
        status_message_.clear();
        headers_.clear();
        body_.clear();
        std::string empty;
        body_stream_.swap_vector(empty);
    }
    
    bool parse_request(session_t &session, parse_callback_t &req_cb) {
        details::request::parser p(session, req_cb);
        return p.parse();
    }

    std::ostream &operator<<(std::ostream &s, response_t &resp) {
        std::map<status_code, std::string>::const_iterator i=details::status_code_msg_map.find(resp.code_);
        if (i==details::status_code_msg_map.end() && resp.status_message_.empty()) {
            // Unknown HTTP status code
            s << "HTTP/1.1 500 Internal Server Error\r\n";
        } else {
            resp.body_.clear();
            resp.body_stream_.swap_vector(resp.body_);
            char buf[100];
            sprintf(buf, "%lu", resp.body_.size());
            
            s << "HTTP/1.1 " << resp.code_ << ' ';
            if (!resp.status_message_.empty()) {
                // Supplied status message
                s << resp.status_message_;
            } else {
                s << i->second << "\r\n";
            }

            s << "Server: " << server_name << "\r\n";
            s << "Content-Length: " << buf << "\r\n";
            for (auto &i : resp.headers_) {
                s << i.first << ": " << i.second << "\r\n";
            }
            s << "\r\n" << resp.body_;
        }
        return s;
    }
    
    bool parse_response(session_t &session, parse_callback_t &&req_cb) {
        // TODO:
        return false;
    }
    
    std::ostream &operator<<(std::ostream &s, request_t &req) {
        // TODO:
        return s;
    }
    
    bool request_callback(session_t &session, request_handler_t &handler) {
        bool ret=true;
        try {
            session.response_.clear();
            // Returning false from handle_request indicates the handler doesn't want the connection to keep alive
            ret = handler(session) && session.keep_alive();
        } catch(...) {
            session.raw_stream() << "HTTP/1.1 500 Internal Server Error\r\n";
        }
        
        if (session.raw()) {
            // Do nothing here
            // Handler handles whole HTTP response by itself, include status, headers, and body
        } else {
            if (ret) {
                char buf[100];
                if (session.max_keepalive()!=0) {
                    if (session.max_keepalive()>session.count_) {
                        // Keep-alive in progress
                        session.response_.headers_.push_back({"Connection", "keep-alive"});
                        sprintf(buf, "timeout=%d, max=%d", session.read_timeout(), session.max_keepalive()-session.count_);
                        session.response_.headers_.push_back({"Keep-Alive", buf});
                    } else {
                        // Max limit reached, stop keeping alive
                        session.response_.headers_.push_back({"Connection", "close"});
                        ret=false;
                    }
                } else {
                    // Limitless keep-alive
                    session.response_.headers_.push_back({"Connection", "keep-alive"});
                    sprintf(buf, "timeout=%d", session.read_timeout());
                    session.response_.headers_.push_back({"Keep-Alive", buf});
                }
            } else {
                session.response_.headers_.push_back({"Connection", "close"});
                ret=false;
            }
            session.raw_stream() << session.response_;
        }
        session.raw_stream().flush();
        session.count_++;
        return ret;
    }
}   // End of namespace http

