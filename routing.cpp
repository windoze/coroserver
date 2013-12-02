//
//  routing.cpp
//  coroserver
//
//  Created by Windoze on 13-12-2.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <memory>
#include <map>
#include <boost/algorithm/string/predicate.hpp>
#include "http_protocol.h"
#include "routing.h"

namespace http {
    routing_pred_t any() {
        return [](session_t &session)->bool{
            return true;
        };
    }
    
    routing_pred_t equals(const std::string &s, bool case_sensitive) {
        if (case_sensitive) {
            return [s](session_t &session)->bool{
                return session.request_.path_==s;
            };
        } else {
            return [s](session_t &session)->bool{
                return boost::algorithm::iequals(session.request_.path_, s);
            };
        }
    }

    routing_pred_t prefix(const std::string &s, bool case_sensitive) {
        if (case_sensitive) {
            return [s](session_t &session)->bool{
                return boost::algorithm::starts_with(session.request_.path_, s);
            };
        } else {
            return [s](session_t &session)->bool{
                return boost::algorithm::istarts_with(session.request_.path_, s);
            };
        }
    }
    
    bool route(routing_table_t &table, session_t &session) {
        for (routing_entry_t &ent : table) {
            if(ent.first(session))
                return ent.second(session);
        }
        return false;
    }
    
    bool router::operator()(session_t &session) {
        return route(table_, session);
    }
    
    void router::operator()(async_tcp_stream &s) {
        protocol_handler(s, *this);
    }
}   // End of namespace http
