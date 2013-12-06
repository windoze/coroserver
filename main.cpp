//
//  main.cpp
//  coroserver
//
//  Created by Windoze on 13-2-23.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <functional>
#include <iostream>
#include "server.h"
#include "http_protocol.h"
#include "routing.h"

#include "condition_variable.hpp"


// Some stupid tests...

typedef int arg_t;

// Test redirection
bool handle_alt_index(http::session_t &session, arg_t &arg) {
    session.response().code(http::SEE_OTHER);
    http::headers_t::const_iterator i=http::find_header(session.request().headers(), "host");
    if (i!=session.request().headers().end()) {
        session.response().headers().push_back(*i);
    }
    session.response().headers().push_back({"Location", "/index.html"});
    return true;
}

// Test stock response
bool handle_not_found(http::session_t &session, arg_t &arg) {
    session.response().code(http::NOT_FOUND);
    return true;
}

// Test normal process
bool handle_index(http::session_t &session, arg_t &arg) {
    using namespace std;
    ostream &ss=session.response().body_stream();
    ss << "<HTML>\r\n<TITLE>Index</TITLE><BODY>\r\n";
    ss << "<H1>This is the index page</H1><HR/>\r\n";
    ss << "<H1>Changing session argument from " << arg << " to " << arg+2 << "</H1><HR/>\r\n";
    arg+=2;
    ss << "<P>" << session.count() << " requests have been processed in this session.<P/>\r\n";
    ss << "<TABLE border=1>\r\n";
    ss << "<TR><TD>Schema</TD><TD>" << session.request().schema() << "</TD></TR>\r\n";
    ss << "<TR><TD>User Info</TD><TD>" << session.request().user_info() << "</TD></TR>\r\n";
    ss << "<TR><TD>Host</TD><TD>" << session.request().host() << "</TD></TR>\r\n";
    ss << "<TR><TD>Port</TD><TD>" << session.request().port() << "</TD></TR>\r\n";
    ss << "<TR><TD>Path</TD><TD>" << session.request().path() << "</TD></TR>\r\n";
    ss << "<TR><TD>Query</TD><TD>" << session.request().query() << "</TD></TR>\r\n";
    ss << "</TABLE>\r\n";
    ss << "<TABLE border=1>\r\n";
    for (auto &h : session.request().headers()) {
        ss << "<TR><TD>" << h.first << "</TD><TD>" << h.second << "</TD></TR>\r\n";
    }
    ss << "</TABLE></BODY></HTML>\r\n";
    return true;
}

// Test deferred process
bool handle_other(http::session_t &session, arg_t &arg) {
    boost::asio::condition_flag flag(session);
    session.strand().post([&session, &flag, &arg](){
        using namespace std;
        ostream &ss=session.response().body_stream();
        ss << "<HTML>\r\n<TITLE>" << session.request().path() << "</TITLE><BODY>\r\n";
        ss << "<H1>" << session.request().path() << "</H1><HR/>\r\n";
        ss << "<H1>Changing session argument from " << arg << " to " << arg+2 << "</H1><HR/>\r\n";
        arg+=2;
        ss << "<P>" << session.count() << " requests have been processed in this session.<P/>\r\n";
        ss << "<TABLE border=1>\r\n";
        ss << "<TR><TD>Schema</TD><TD>" << session.request().schema() << "</TD></TR>\r\n";
        ss << "<TR><TD>User Info</TD><TD>" << session.request().user_info() << "</TD></TR>\r\n";
        ss << "<TR><TD>Host</TD><TD>" << session.request().host() << "</TD></TR>\r\n";
        ss << "<TR><TD>Port</TD><TD>" << session.request().port() << "</TD></TR>\r\n";
        ss << "<TR><TD>Path</TD><TD>" << session.request().path() << "</TD></TR>\r\n";
        ss << "<TR><TD>Query</TD><TD>" << session.request().query() << "</TD></TR>\r\n";
        ss << "</TABLE>\r\n";
        ss << "<TABLE border=1>\r\n";
        for (auto &h : session.request().headers()) {
            ss << "<TR><TD>" << h.first << "</TD><TD>" << h.second << "</TD></TR>\r\n";
        }
        ss << "</TABLE></BODY></HTML>\r\n";
        flag=true;
    });
    flag.wait();
    return true;
}

// Test client connection
bool handle_proxy(http::session_t &session, arg_t &arg) {
    session.raw(true);
    async_tcp_stream s(session.yield_context(), "u", "80");
    session.request().body_stream() << session.request().body();
    s << session.request();
    s >> session.response();
    session.raw_stream() << session.response();
    session.raw_stream().flush();
    return true;
}

int main(int argc, const char *argv[]) {
    std::size_t num_threads = 3;
    try {
        http::protocol_handler<> hh;
        
        http::protocol_handler<arg_t> handler;
        handler.set_default_argument(42);
        handler.set_open_handler([](http::session_t &session, arg_t &arg)->bool{
            session.read_timeout(5);
            session.write_timeout(5);
            session.max_keepalive(3);
            return true;
        });
        handler.set_request_handler(http::router<arg_t>({
            {http::url_equals("/"), &handle_alt_index},
            {http::url_equals("/index.html"), &handle_index},
            {http::url_starts_with("/index") && http::url_ends_with(".htm"), &handle_alt_index},
            {http::url_starts_with("/ptest/"), &handle_proxy},
            {http::url_equals("/favicon.ico"), &handle_not_found},
            {http::any(), &handle_other},
        }));
        server s({{handler, {"0::0", "20000"}}, {hh, {"0::0", "20001"}}},
                 num_threads);
        s();
    } catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << "\n";
    }
    return 0;
}
