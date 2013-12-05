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
        
        const std::map<method, std::string> method_name_map={
            {DELETE, "DELETE"},
            {GET, "GET"},
            {HEAD, "HEAD"},
            {POST, "POST"},
            {PUT, "PUT"},
            /* pathological */
            {CONNECT, "CONNECT"},
            {OPTIONS, "OPTIONS"},
            {TRACE, "TRACE"},
            /* webdav */
            {COPY, "COPY"},
            {LOCK, "LOCK"},
            {MKCOL, "MKCOL"},
            {MOVE, "MOVE"},
            {PROPFIND, "PROPFIND"},
            {PROPPATCH, "PROPPATCH"},
            {SEARCH, "SEARCH"},
            {UNLOCK, "UNLOCK"},
            /* subversion */
            {REPORT, "REPORT"},
            {MKACTIVITY, "MKACTIVITY"},
            {CHECKOUT, "CHECKOUT"},
            {MERGE, "MERGE"},
            /* upnp */
            {MSEARCH, "MSEARCH"},
            {NOTIFY, "NOTIFY"},
            {SUBSCRIBE, "SUBSCRIBE"},
            {UNSUBSCRIBE, "UNSUBSCRIBE"},
            /* RFC-5789 */
            {PATCH, "PATCH"},
            {PURGE, "PURGE"},
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
                
                request_t &req() { return session_.request(); }
                
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
                        req().headers().rbegin()->first.append(at, length);
                    } else {
                        req().headers().push_back(header_t());
                        req().headers().rbegin()->first.reserve(256);
                        req().headers().rbegin()->first.assign(at, length);
                    }
                    state_=field;
                    return 0;
                }
                int on_header_value(const char *at, size_t length) {
                    if (state_==value)
                        req().headers().rbegin()->second.append(at, length);
                    else {
                        req().headers().rbegin()->second.reserve(256);
                        req().headers().rbegin()->second.assign(at, length);
                    }
                    state_=value;
                    return 0;
                }
                int on_headers_complete() {
                    return 0;
                }
                int on_body(const char *at, size_t length) {
                    req().body_stream().write(at, length);
                    state_=body;
                    return 0;
                }
                int on_message_complete() {
                    state_=end;
                    req().method((method)(parser_.method));
                    req().http_major(parser_.http_major);
                    req().http_minor(parser_.http_minor);
                    http_parser_url u;
                    http_parser_parse_url(url_.c_str(),
                                          url_.size(),
                                          req().method()==CONNECT,
                                          &u);
                    // Components for proxy requests
                    // NOTE: Schema, user info, host, and port may only exist in proxy requests
                    if(u.field_set & 1 << UF_SCHEMA) {
                        req().schema(std::string(url_.begin()+u.field_data[UF_SCHEMA].off,
                                                 url_.begin()+u.field_data[UF_SCHEMA].off+u.field_data[UF_SCHEMA].len));
                    }
                    if(u.field_set & 1 << UF_USERINFO) {
                        req().user_info(std::string(url_.begin()+u.field_data[UF_USERINFO].off,
                                                    url_.begin()+u.field_data[UF_USERINFO].off+u.field_data[UF_USERINFO].len));
                    }
                    if(u.field_set & 1 << UF_HOST) {
                        req().host(std::string(url_.begin()+u.field_data[UF_HOST].off,
                                               url_.begin()+u.field_data[UF_HOST].off+u.field_data[UF_HOST].len));
                    }
                    if(u.field_set & 1 << UF_PORT) {
                        req().port(u.port);
                    } else {
                        req().port(0);
                    }
                    // Common components
                    if(u.field_set & 1 << UF_PATH) {
                        req().path(std::string(url_.begin()+u.field_data[UF_PATH].off,
                                               url_.begin()+u.field_data[UF_PATH].off+u.field_data[UF_PATH].len));
                    }
                    if(u.field_set & 1 << UF_QUERY) {
                        req().query(std::string(url_.begin()+u.field_data[UF_QUERY].off,
                                                url_.begin()+u.field_data[UF_QUERY].off+u.field_data[UF_QUERY].len));
                    }
                    req().keep_alive(http_should_keep_alive(&parser_));
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
            struct parser {
                enum parser_state{
                    none,
                    start,
                    status,
                    field,
                    value,
                    body,
                    end
                };
                
                response_t &resp() { return response_; }
                
                int on_message_begin() {
                    resp().clear();
                    state_=start;
                    return 0;
                }
                int on_url(const char *at, size_t length) {
                    return 0;
                }
                int on_status_complete() {
                    return 0;
                }
                int on_header_field(const char *at, size_t length) {
                    if (state_==field) {
                        resp().headers().rbegin()->first.append(at, length);
                    } else {
                        resp().headers().push_back(header_t());
                        resp().headers().rbegin()->first.reserve(256);
                        resp().headers().rbegin()->first.assign(at, length);
                    }
                    state_=field;
                    return 0;
                }
                int on_header_value(const char *at, size_t length) {
                    if (state_==value)
                        resp().headers().rbegin()->second.append(at, length);
                    else {
                        resp().headers().rbegin()->second.reserve(256);
                        resp().headers().rbegin()->second.assign(at, length);
                    }
                    state_=value;
                    return 0;
                }
                int on_headers_complete() {
                    return 0;
                }
                int on_body(const char *at, size_t length) {
                    resp().body_stream().write(at, length);
                    state_=body;
                    return 0;
                }
                int on_message_complete() {
                    state_=end;
                    resp().http_major(parser_.http_major);
                    resp().http_minor(parser_.http_minor);
                    resp().code(status_code(parser_.status_code));
                    resp().keep_alive(http_should_keep_alive(&parser_));
                    should_continue_=false;
                    return 0;
                }

                std::istream &is_;
                response_t &response_;
                http_parser parser_;
                std::string url_;
                parser_state state_;
                bool should_continue_;
                
                parser(std::istream &is, response_t &resp);
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
            
            parser::parser(std::istream &is, response_t &resp)
            : is_(is)
            , response_(resp)
            {
                parser_.data=reinterpret_cast<void*>(this);
                http_parser_init(&parser_, HTTP_RESPONSE);
            }
            
            bool parser::parse() {
                should_continue_=true;
                state_=none;
                constexpr int buf_size=1024;
                char buf[buf_size];
                int recved=0;
                int nparsed=0;
                while (is_) {
                    // Read some data
                    recved = is_.readsome(buf, buf_size);
                    if (recved<=0) {
                        // Connection closed
                        return true;
                    }
                    nparsed=http_parser_execute(&parser_, &settings_, buf, recved);
                    if (!should_continue_) {
                        // TODO: Should I put extra data back into stream?
                        break;
                    }
                    if (nparsed!=recved) {
                        // Parse error
                        return false;
                    }
                }
                return true;
            }
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
        // NOTE: Why there is no clear() in ovectorstream?
        std::string empty;
        body_stream().swap_vector(empty);
    }
    
    void response_t::clear() {
        http_major_=0;
        http_minor_=0;
        code_=OK;
        status_message_.clear();
        headers_.clear();
        keep_alive_=false;
        // NOTE: Why there is no clear() in ovectorstream?
        std::string empty;
        body_stream().swap_vector(empty);
    }
    
    bool parse_request(session_t &session, parse_callback_t &req_cb) {
        details::request::parser p(session, req_cb);
        return p.parse();
    }
    
    bool request_callback(session_t &session, request_handler_t &handler) {
        bool ret=true;
        try {
            session.response().clear();
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
                    if (session.max_keepalive()>session.count()) {
                        // Keep-alive in progress
                        session.response().headers().push_back({"Connection", "keep-alive"});
                        sprintf(buf, "timeout=%d, max=%d", session.read_timeout(), session.max_keepalive()-session.count());
                        session.response().headers().push_back({"Keep-Alive", buf});
                    } else {
                        // Max limit reached, stop keeping alive
                        session.response().headers().push_back({"Connection", "close"});
                        ret=false;
                    }
                } else {
                    // Limitless keep-alive
                    session.response().headers().push_back({"Connection", "keep-alive"});
                    sprintf(buf, "timeout=%d", session.read_timeout());
                    session.response().headers().push_back({"Keep-Alive", buf});
                }
            } else {
                session.response().headers().push_back({"Connection", "close"});
                ret=false;
            }
            session.raw_stream() << session.response();
        }
        session.raw_stream().flush();
        session.inc_count();
        return ret;
    }

    std::ostream &operator<<(std::ostream &s, response_t &resp) {
        std::map<status_code, std::string>::const_iterator i=details::status_code_msg_map.find(resp.code());
        if (i==details::status_code_msg_map.end() && resp.status_message().empty()) {
            // Unknown HTTP status code
            s << "HTTP/1.1 500 Internal Server Error\r\n";
        } else {
            char buf[100];
            sprintf(buf, "%lu", resp.body().size());
            
            s << "HTTP/1.1 " << resp.code() << ' ';
            if (!resp.status_message().empty()) {
                // Supplied status message
                s << resp.status_message();
            } else {
                s << i->second << "\r\n";
            }

            bool server_found=false;
            bool content_len_found=false;
            for (auto &i : resp.headers()) {
                s << i.first << ": " << i.second << "\r\n";
                if (boost::algorithm::iequals(i.first, "server")) server_found=true;
                if (boost::algorithm::iequals(i.first, "content-length")) content_len_found=true;
            }
            if (!server_found) s << "Server: " << server_name << "\r\n";
            if (!content_len_found) s << "Content-Length: " << buf << "\r\n";
            s << "\r\n" << resp.body();
        }
        return s;
    }
    
    // Client side
    
    // Send request
    std::ostream &operator<<(std::ostream &s, request_t &req) {
        std::map<method, std::string>::const_iterator i=details::method_name_map.find(req.method());
        if (i==details::method_name_map.end()) {
            // Unknow method
            return s;
        }

        s << i->second << ' ' << req.path();
        if (!req.query().empty()) {
            s << '?' << req.query();
        }
        s << " HTTP/" << req.http_major() << '.' << req.http_minor() << "\r\n";
        for (header_t &h : req.headers()) {
            s << h.first << ": " << h.second << "\r\n";
        }
        s << "Content-Length: " << req.body().size() << "\r\n";
        if (req.keep_alive()) {
            s << "Connection: keep-alive\r\n";
        } else {
            s << "Connection: close\r\n";
        }
        s << "\r\n";
        if (!req.body().empty()) {
            s << req.body();
        }
        s.flush();
        return s;
    }
    
    // Parse response
    bool parse_response(std::istream &is, response_t &resp) {
        details::response::parser p(is, resp);
        return p.parse();
    }
}   // End of namespace http

