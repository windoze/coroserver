/*//////////////////////////////////////////////////////////////////////////////
 Copyright (c) 2013 Jamboree
 
 Distributed under the Boost Software License, Version 1.0. (See accompanying
 file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 //////////////////////////////////////////////////////////////////////////////*/
#ifndef BOOST_ASIO_CONDITION_VARIABLE_HPP_INCLUDED
#define BOOST_ASIO_CONDITION_VARIABLE_HPP_INCLUDED


#include <deque>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>

namespace boost {
    namespace asio {
        class condition_variable {
            struct handler_t {
                explicit handler_t(yield_context const &ctx)
                : coro_(ctx.coro_.lock())
                {}
                
                void operator()()
                { (*coro_)(); }
                
                detail::shared_ptr<yield_context::callee_type> coro_;
            };
            
        public:
            explicit condition_variable(strand &strand)
            : strand_(strand)
            {}
            
            void wait(yield_context const &ctx) {
                suspended_.push_back(handler_t(ctx));
                ctx.ca_();
            }
            
            void notify_one() {
                if (suspended_.empty()) return;
                strand_.post(suspended_.front());
                suspended_.pop_front();
            }
            
            void notify_all() {
                for (handler_t &h : suspended_) strand_.post(h);
                suspended_.clear();
            }
            
        private:
            strand &strand_;
            std::deque<handler_t> suspended_;
        };
        
        struct condition_flag {
            explicit condition_flag(strand &s)
            : b_()
            , cond_(s)
            {}
            
            void wait(yield_context const &ctx)
            { while (!b_) cond_.wait(ctx); }
            
            condition_flag &operator=(bool b) {
                if ((b_ = b)==true) cond_.notify_all();
                return *this;
            }
            
        private:
            bool b_;
            condition_variable cond_;
        };
    }   // End of namespace asio
}   // End of namespace boost
#endif
