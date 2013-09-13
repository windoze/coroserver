//
//  http_protocol.cpp
//  coroserver
//
//  Created by Windoze on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <iostream>
#include <vector>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include "http_protocol.h"

bool http_protocol(async_tcp_stream &s) {
    using namespace std;
    using namespace boost;
    string line;
    
    // Read request line
    getline(s, line);
    
    // TODO: Parse request line
    //typedef vector< iterator_range<string::iterator> > find_vector_type;
    //find_vector_type FindVec;
    //find_all(FindVec, line, " ");
    // Read headers
    do {
        getline(s, line);
        trim(line);
        // TODO: Parse headers
    } while (!line.empty());
    
    // TODO: Read and parse request body
    
    // TODO: Generate response
    s <<
    "HTTP/1.1 200 OK\n"
    "Content-Type: text/html\n"
    "Content-Length: 65\n"
    "\n"
    "<HTML>\n<TITLE>Test</TITLE><BODY><H1>It works!</H1></BODY></HTML>\n";
    return false;
}
