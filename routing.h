//
//  routing.h
//  coroserver
//
//  Created by Windoze on 13-12-2.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef __coroserver__routing__
#define __coroserver__routing__

#include <map>
#include <functional>
#include <string>
#include <list>
#include "async_stream.h"

namespace http {
    struct session_t;
    typedef std::function<bool(session_t &)> routing_pred_t;
    typedef std::function<bool(session_t &)> request_handler_t;
    typedef std::pair<routing_pred_t, request_handler_t> routing_entry_t;
    typedef std::list<routing_entry_t> routing_table_t;
    
    routing_pred_t any();
    routing_pred_t equals(const std::string &s, bool case_sensitive=false);
    routing_pred_t prefix(const std::string &s, bool case_sensitive=false);

    bool route_request(routing_table_t &table, session_t &session);
    
    struct router {
        router(const routing_table_t &table)
        : table_(table)
        {}
        
        router(routing_table_t &&table)
        : table_(std::move(table))
        {}
        
        bool operator()(session_t &session);
        void operator()(async_tcp_stream &s);
        
        routing_table_t table_;
    };
}   // End of namespace http

#endif /* defined(__coroserver__routing__) */
