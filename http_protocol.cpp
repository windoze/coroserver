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
    namespace details {
        const std::map<int, std::string> status_code_msg_map={
            {100, "Continue"},
            {101, "Switching Protocols"},
            {200, "OK"},
            {201, "Created"},
            {202, "Accepted"},
            {203, "Non-Authoritative Information"},
            {204, "No Content"},
            {205, "Reset Content"},
            {206, "Partial Content"},
            {300, "Multiple Choices"},
            {301, "Moved Permanently"},
            {302, "Found"},
            {303, "See Other"},
            {304, "Not Modified"},
            {307, "Temporary Redirect"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {402, "Payment Required"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {406, "Not Acceptable"},
            {407, "Proxy Authentication Required"},
            {408, "Request Timeout"},
            {409, "Conflict"},
            {410, "Gone"},
            {411, "Length Required"},
            {412, "Precondition Failed"},
            {413, "Request Entity Too Large"},
            {414, "Request-URI Too Long"},
            {415, "Unsupported Media Type"},
            {416, "Requested Range Not Satisfiable"},
            {417, "Expectation Failed"},
            {500, "Internal Server Error"},
            {501, "Not Implemented"},
            {502, "Bad Gateway"},
            {503, "Service Unavailable"},
            {504, "Gateway Timeout"},
            {505, "HTTP Version Not Supported"},
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
                http_parser_settings settings_;
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
            
            parser::parser() {
                parser_.data=reinterpret_cast<void*>(this);
                settings_.on_message_begin=http::details::request::on_message_begin;
                settings_.on_url=http::details::request::on_url;
                settings_.on_status_complete=http::details::request::on_status_complete;
                settings_.on_header_field=http::details::request::on_header_field;
                settings_.on_header_value=http::details::request::on_header_value;
                settings_.on_headers_complete=http::details::request::on_headers_complete;
                settings_.on_body=http::details::request::on_body;
                settings_.on_message_complete=http::details::request::on_message_complete;
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

    bool handle_request(const http::request_t &req, http::response_t &resp) {
        using namespace std;
        ostream &ss=resp.body_stream_;
        resp.headers_.push_back(http::header_t("Server", "coroserver 0.1"));
        ss << "<HTML>\r\n<TITLE>Test</TITLE><BODY>\r\n";
        ss << "<TABLE border=1>\r\n";
        ss << "<TR><TD>Schema</TD><TD>" << req.schema_ << "</TD></TR>\r\n";
        ss << "<TR><TD>User Info</TD><TD>" << req.user_info_ << "</TD></TR>\r\n";
        ss << "<TR><TD>Host</TD><TD>" << req.host_ << "</TD></TR>\r\n";
        ss << "<TR><TD>Port</TD><TD>" << req.port_ << "</TD></TR>\r\n";
        ss << "<TR><TD>Path</TD><TD>" << req.path_ << "</TD></TR>\r\n";
        ss << "<TR><TD>Query</TD><TD>" << req.query_ << "</TD></TR>\r\n";
        ss << "</TABLE>\r\n";
        ss << "<TABLE border=1>\r\n";
        for (auto &h : req.headers_) {
            ss << "<TR><TD>" << h.first << "</TD><TD>" << h.second << "</TD></TR>\r\n";
        }
        ss << "</TABLE></BODY></HTML>\r\n";
        
        return true;
    }

    bool protocol_handler(async_tcp_stream &s) {
        using namespace std;
        using namespace boost;
        
        bool keep_alive=false;
        do {
            http::request_t req;
            if(!http::parse(s, req)) {
                s << "HTTP/1.1 400 Bad request\r\n";
                return false;
            }
            
            if (req.closed_) {
                return false;
            }
            
            boost::interprocess::basic_ovectorstream<std::string> ss;
            http::response_t resp(ss, s);
            
            // Returning false from handle_request indicates the handler doesn't want the connection to keep alive
            try {
                keep_alive = req.keep_alive_ && !(handle_request(req, resp));
            } catch(...) {
                s << "HTTP/1.1 500 Internal Server Error\r\n";
                break;
            }
            
            if (resp.raw_) {
                // Do nothing here
                // Handler handles whole HTTP response by itself, include status, headers, and body
            } else {
                string out_buf;
                ss.swap_vector(out_buf);
                
                std::map<int, std::string>::const_iterator i=details::status_code_msg_map.find(resp.code_);
                if (i==details::status_code_msg_map.end()) {
                    s << "HTTP/1.1 500 Internal Server Error\r\n";
                    break;
                }
                s << "HTTP/1.1 " << resp.code_ << ' ' << i->second << "\r\n";
                for (auto &i : resp.headers_) {
                    s << i.first << ": " << i.second << "\r\n";
                }
                if (keep_alive) {
                    s << "Connection: keep-alive\r\n";
                } else {
                    s << "Connection: close\r\n";
                }
                s << "Content-Length: " << out_buf.size() << "\r\n\r\n";
                s << out_buf;
            }
            s.flush();
        } while (keep_alive);
        
        s.flush();
        return false;
    }
}   // End of namespace http

