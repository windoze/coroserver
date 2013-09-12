//
//  http_session.cpp
//  coroserver
//
//  Created by Chen Xu on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include "http_session.h"

http_session::http_session(std::istream &is, std::ostream &os, boost::asio::yield_context yield)
: is_(is)
, os_(os)
, yield_(yield)
{}

bool http_session::operator()() {
    using namespace std;
    using namespace boost;
    string line;
    // Read request line
    getline(is_, line);
    // TODO: Parse request line
    //typedef vector< iterator_range<string::iterator> > find_vector_type;
    //find_vector_type FindVec;
    //find_all(FindVec, line, " ");
    // Read headers
    do {
        getline(is_, line);
        trim(line);
        // TODO: Parse headers
    } while (!line.empty());
    // TODO: 
    os_ << "HTTP/1.1 200 OK\n";
    os_ << "Content-Type: text/html\n";
    os_ << "Content-Length: 67\n";
    os_ << "\n";
    os_ << "<HTML>\n<TITLE>Test</TITLE><BODY><H1>It works!</H1></BODY></HTML>\n";
    os_.flush();
    return false;
}
