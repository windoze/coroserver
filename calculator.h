//
//  stream_calc.h
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#ifndef stream_calc_h_included
#define stream_calc_h_included

#include <istream>
#include <ostream>
#include <boost/rational.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#include "async_stream.h"

namespace calculator {
    /*
     * Line-oriented calculator protocol
     */
    void protocol_handler(net::async_tcp_stream &s);
}

#endif /* defined(stream_calc_h_included) */
