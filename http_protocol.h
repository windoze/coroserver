//
//  http_protocol.h
//  coroserver
//
//  Created by Windoze on 13-9-11.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <iostream>
#include "async_stream.h"

#ifndef __coroserver__http_protocol__
#define __coroserver__http_protocol__

/**
 * Handle HTTP protocol
 *
 * @param s the socket stream
 * @param yield the yield_context, can be used to create new connections
 */
bool http_protocol(async_tcp_stream& s);

#endif /* defined(__coroserver__http_protocol__) */
