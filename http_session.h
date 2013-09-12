//
//  http_session.h
//  coroserver
//
//  Created by Chen Xu on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <istream>
#include <ostream>
#include <boost/asio/spawn.hpp>

#ifndef __coroserver__http_session__
#define __coroserver__http_session__

class http_session {
public:
    http_session(std::istream &is, std::ostream &os, boost::asio::yield_context yield);
    bool operator()();
    
private:
    std::istream &is_;
    std::ostream &os_;
    boost::asio::yield_context yield_;
};

#endif /* defined(__coroserver__http_session__) */
