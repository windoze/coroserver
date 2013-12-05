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
    
    routing_pred_t url_equals(const std::string &s, bool case_sensitive) {
        if (case_sensitive) {
            return [s](session_t &session)->bool{
                return session.request().path()==s;
            };
        } else {
            return [s](session_t &session)->bool{
                return boost::algorithm::iequals(session.request().path(), s);
            };
        }
    }

    routing_pred_t url_starts_with(const std::string &s, bool case_sensitive) {
        if (case_sensitive) {
            return [s](session_t &session)->bool{
                return boost::algorithm::starts_with(session.request().path(), s);
            };
        } else {
            return [s](session_t &session)->bool{
                return boost::algorithm::istarts_with(session.request().path(), s);
            };
        }
    }
    
    routing_pred_t url_ends_with(const std::string &s, bool case_sensitive) {
        if (case_sensitive) {
            return [s](session_t &session)->bool{
                return boost::algorithm::ends_with(session.request().path(), s);
            };
        } else {
            return [s](session_t &session)->bool{
                return boost::algorithm::iends_with(session.request().path(), s);
            };
        }
    }
    
    routing_pred_t operator &&(routing_pred_t c1, routing_pred_t c2) {
        return [c1, c2](session_t &session)->bool {
            return c1(session) && c2(session);
        };
    }

    routing_pred_t operator ||(routing_pred_t c1, routing_pred_t c2) {
        return [c1, c2](session_t &session)->bool {
            return c1(session) || c2(session);
        };
    }

    routing_pred_t operator !(routing_pred_t c) {
        return [c](session_t &session)->bool {
            return !c(session);
        };
    }
}   // End of namespace http
