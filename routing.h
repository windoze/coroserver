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
#include <memory>

namespace http {
    struct session_t;
    typedef std::function<bool(session_t &)> routing_pred_t;
    
    routing_pred_t any();
    routing_pred_t url_equals(const std::string &s, bool case_sensitive=false);
    routing_pred_t url_starts_with(const std::string &s, bool case_sensitive=false);
    routing_pred_t url_ends_with(const std::string &s, bool case_sensitive=false);
    routing_pred_t operator &&(routing_pred_t c1, routing_pred_t c2);
    routing_pred_t operator ||(routing_pred_t c1, routing_pred_t c2);
    routing_pred_t operator !(routing_pred_t c);
    
    template<typename Arg>
    struct router {
        typedef std::function<bool(session_t &, Arg &)> handler_t;
        typedef std::pair<routing_pred_t, handler_t> routing_entry_t;
        typedef std::list<routing_entry_t> routing_table_t;
        typedef std::shared_ptr<routing_table_t> routing_table_ptr;

        router(const routing_table_t &table)
        : table_(new routing_table_t(table))
        {}
        
        router(routing_table_t &&table)
        : table_(new routing_table_t(std::move(table)))
        {}
        
        bool operator()(session_t &session, Arg &arg) {
            for (routing_entry_t &ent : *table_) {
                if(ent.first(session))
                    return ent.second(session, arg);
            }
            return false;
        }
        
        routing_table_ptr table_;
    };

    template<>
    struct router<void> {
        typedef std::function<bool(session_t &)> handler_t;
        typedef std::pair<routing_pred_t, handler_t> routing_entry_t;
        typedef std::list<routing_entry_t> routing_table_t;
        typedef std::shared_ptr<routing_table_t> routing_table_ptr;
        
        router(const routing_table_t &table)
        : table_(new routing_table_t(table))
        {}
        
        router(routing_table_t &&table)
        : table_(new routing_table_t(std::move(table)))
        {}
        
        bool operator()(session_t &session) {
            for (routing_entry_t &ent : *table_) {
                if(ent.first(session))
                    return ent.second(session);
            }
            return false;
        }
        
        routing_table_ptr table_;
    };
}   // End of namespace http

#endif /* defined(__coroserver__routing__) */
