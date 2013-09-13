//
//  main.cpp
//  coroserver
//
//  Created by Windoze on 13-2-23.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <iostream>
#include "http_protocol.h"
#include "server.h"

int main(int argc, const char *argv[]) {
    const char *addr="0.0.0.0";
    const char *port="20000";
    std::size_t num_threads = 3;
    try {
        server s(&http_protocol, addr, port, num_threads);
        s();
    } catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << "\n";
    }
    return 0;
}
