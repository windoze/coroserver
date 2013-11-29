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
                    url,
                    field,
                    value,
                    body,
                    end
                };
                
                int on_message_begin() {
                    url_.clear();
                    state_=none;
                    completed_=false;
                    return 0;
                }
                int on_url(const char *at, size_t length) {
                    if(state_!=url)
                        url_.assign(at, length);
                    else
                        url_.append(at, length);
                    state_=url;
                    return 0;
                }
                int on_status_complete() { return 0; }
                int on_header_field(const char *at, size_t length) {
                    if (state_!=field) {
                        req_->headers_.push_back(header_t());
                        req_->headers_.rbegin()->first.assign(at, length);
                    } else {
                        req_->headers_.rbegin()->first.append(at, length);
                    }
                    state_=field;
                    return 0;
                }
                int on_header_value(const char *at, size_t length) {
                    if (state_!=value)
                        req_->headers_.rbegin()->second.assign(at, length);
                    else
                        req_->headers_.rbegin()->second.append(at, length);
                    state_=value;
                    return 0;
                }
                int on_headers_complete() { return 0; }
                int on_body(const char *at, size_t length) {
                    if (state_!=body)
                        req_->body_.assign(at, length);
                    else
                        req_->body_.append(at, length);
                    state_=body;
                    return 0;
                }
                int on_message_complete() {
                    completed_=true;
                    state_=end;
                    req_->method_=(http::method)(parser_.method);
                    req_->http_major_=parser_.http_major;
                    req_->http_minor_=parser_.http_minor;
                    http_parser_url u;
                    http_parser_parse_url(url_.c_str(),
                                          url_.size(),
                                          req_->method_==http::CONNECT,
                                          &u);
                    // Components for proxy requests
                    // NOTE: Schema, user info, host, and port may only exist in proxy requests
                    if(u.field_set & 1 << UF_SCHEMA) {
                        req_->schema_.assign(url_.begin()+u.field_data[UF_SCHEMA].off,
                                             url_.begin()+u.field_data[UF_SCHEMA].off+u.field_data[UF_SCHEMA].len);
                    }
                    if(u.field_set & 1 << UF_USERINFO) {
                        req_->user_info_.assign(url_.begin()+u.field_data[UF_USERINFO].off,
                                                url_.begin()+u.field_data[UF_USERINFO].off+u.field_data[UF_USERINFO].len);
                    }
                    if(u.field_set & 1 << UF_HOST) {
                        req_->host_.assign(url_.begin()+u.field_data[UF_HOST].off,
                                           url_.begin()+u.field_data[UF_HOST].off+u.field_data[UF_HOST].len);
                    }
                    if(u.field_set & 1 << UF_PORT) {
                        req_->port_=u.port;
                    } else {
                        req_->port_=0;
                    }
                    // Common components
                    if(u.field_set & 1 << UF_PATH) {
                        req_->path_.assign(url_.begin()+u.field_data[UF_PATH].off,
                                           url_.begin()+u.field_data[UF_PATH].off+u.field_data[UF_PATH].len);
                    }
                    if(u.field_set & 1 << UF_QUERY) {
                        req_->query_.assign(url_.begin()+u.field_data[UF_QUERY].off,
                                            url_.begin()+u.field_data[UF_QUERY].off+u.field_data[UF_QUERY].len);
                    }
                    return 0;
                }
                
                http_parser parser_;
                std::string url_;
                bool completed_;
                request_t *req_;
                parser_state state_;
                
                parser();
                bool parse(std::istream &is, request_t &req);
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
                &http::details::request::on_message_begin,
                &http::details::request::on_url,
                &http::details::request::on_status_complete,
                &http::details::request::on_header_field,
                &http::details::request::on_header_value,
                &http::details::request::on_headers_complete,
                &http::details::request::on_body,
                &http::details::request::on_message_complete,
            };
            
            parser::parser() {
                parser_.data=reinterpret_cast<void*>(this);
                http_parser_init(&parser_, HTTP_REQUEST);
            }
            
            bool parser::parse(std::istream &is, http::request_t &req) {
                state_=none;
                req.closed_=false;
                constexpr int buf_size=1024;
                char buf[buf_size];
                int recved=0;
                int nparsed=0;
                req_=&req;
                while (is) {
                    // Read some data
                    recved = is.readsome(buf, buf_size);
                    if (recved<=0) {
                        // Connection closed
                        req.closed_=true;
                        return true;
                    }
                    nparsed=http_parser_execute(&parser_, &settings_, buf, recved);
                    if (nparsed!=recved) {
                        // Parse error
                        return false;
                    }
                    if (completed_) {
                        break;
                    }
                }
                // Finishing
                req_->keep_alive_=http_should_keep_alive(&parser_);
                
                return completed_;
            }
        }   // End of namespace request
    }   // End of namespace details
    
    bool parse(std::istream &is, request_t &req) {
        details::request::parser p;
        return p.parse(is, req);
    }

    bool protocol_handler(async_tcp_stream_ptr s) {
        using namespace std;
        using namespace boost;
        bool handle_request(http::request_ptr req, http::response_ptr resp);
        
        bool keep_alive=false;
        
        do {
            http::request_ptr req(new http::request_t);
            if(!http::parse(*s, *req)) {
                *s << "HTTP/1.1 400 Bad request\r\n";
                return false;
            }
            
            if (req->closed_) {
                return false;
            }
            
            boost::interprocess::basic_ovectorstream<std::string> ss;
            http::response_ptr resp(new response_t(ss, s));
            
            // Returning false from handle_request indicates the handler doesn't want the connection to keep alive
            try {
                keep_alive = handle_request(req, resp) && req->keep_alive_;
            } catch(...) {
                *s << "HTTP/1.1 500 Internal Server Error\r\n";
                break;
            }
            
            if (resp->raw_) {
                // Do nothing here
                // Handler handles whole HTTP response by itself, include status, headers, and body
            } else {
                string out_buf;
                ss.swap_vector(out_buf);
                
                std::map<status_code, std::string>::const_iterator i=details::status_code_msg_map.find(resp->code_);
                if (i==details::status_code_msg_map.end()) {
                    *s << "HTTP/1.1 500 Internal Server Error\r\n";
                    break;
                }
                *s << "HTTP/1.1 " << resp->code_ << ' ' << i->second << "\r\n";
                *s << "Server: " << server_name << "\r\n";
                for (auto &i : resp->headers_) {
                    *s << i.first << ": " << i.second << "\r\n";
                }
                if (keep_alive) {
                    *s << "Connection: keep-alive\r\n";
                } else {
                    *s << "Connection: close\r\n";
                }
                *s << "Content-Length: " << out_buf.size() << "\r\n\r\n";
                *s << out_buf;
            }
            s->flush();
        } while (keep_alive);
        
        s->flush();
        return false;
    }

    bool handle_request(http::request_ptr req, http::response_ptr resp) {
        if(1) {
            resp->raw_stream_->spawn([req, resp](boost::asio::yield_context yield){
                using namespace std;
                ostream &ss=resp->body_stream_;
                ss << "<HTML>\r\n<TITLE>Test</TITLE><BODY>\r\n";
                ss << "<TABLE border=1>\r\n";
                ss << "<TR><TD>Schema</TD><TD>" << req->schema_ << "</TD></TR>\r\n";
                ss << "<TR><TD>User Info</TD><TD>" << req->user_info_ << "</TD></TR>\r\n";
                ss << "<TR><TD>Host</TD><TD>" << req->host_ << "</TD></TR>\r\n";
                ss << "<TR><TD>Port</TD><TD>" << req->port_ << "</TD></TR>\r\n";
                ss << "<TR><TD>Path</TD><TD>" << req->path_ << "</TD></TR>\r\n";
                ss << "<TR><TD>Query</TD><TD>" << req->query_ << "</TD></TR>\r\n";
                ss << "</TABLE>\r\n";
                ss << "<TABLE border=1>\r\n";
                for (auto &h : req->headers_) {
                    ss << "<TR><TD>" << h.first << "</TD><TD>" << h.second << "</TD></TR>\r\n";
                }
                ss << "</TABLE></BODY></HTML>\r\n";
                
                return true;
            });
        } else {
            using namespace std;
            ostream &ss=resp->body_stream_;
            ss << "<HTML>\r\n<TITLE>Test</TITLE><BODY>\r\n";
            ss << "<TABLE border=1>\r\n";
            ss << "<TR><TD>Schema</TD><TD>" << req->schema_ << "</TD></TR>\r\n";
            ss << "<TR><TD>User Info</TD><TD>" << req->user_info_ << "</TD></TR>\r\n";
            ss << "<TR><TD>Host</TD><TD>" << req->host_ << "</TD></TR>\r\n";
            ss << "<TR><TD>Port</TD><TD>" << req->port_ << "</TD></TR>\r\n";
            ss << "<TR><TD>Path</TD><TD>" << req->path_ << "</TD></TR>\r\n";
            ss << "<TR><TD>Query</TD><TD>" << req->query_ << "</TD></TR>\r\n";
            ss << "</TABLE>\r\n";
            ss << "<TABLE border=1>\r\n";
            for (auto &h : req->headers_) {
                ss << "<TR><TD>" << h.first << "</TD><TD>" << h.second << "</TD></TR>\r\n";
            }
            ss << "</TABLE></BODY></HTML>\r\n";
        }
        return true;
    }
}   // End of namespace http

