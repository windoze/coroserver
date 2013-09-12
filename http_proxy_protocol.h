//
//  http_proxy_protocol.h
//  coroserver
//
//  Created by Chen Xu on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <iostream>
#include <boost/asio/spawn.hpp>

#ifndef __coroserver__http_proxy_protocol__
#define __coroserver__http_proxy_protocol__

bool http_proxy_protocol(std::iostream &s, boost::asio::yield_context yield);

#endif /* defined(__coroserver__http_proxy_protocol__) */
