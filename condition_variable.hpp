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
                explicit handler_t(yield_context const ctx)
                : coro_(ctx.coro_)
                {}
                
                void operator()() {
                    auto h(coro_.lock());
                    if(h) (*h)();
                }
                
                detail::weak_ptr<yield_context::callee_type> coro_;
            };
            
        public:
            explicit condition_variable(yield_context ctx)
            : ctx_(ctx)
            {}
            
            /**
             * Wait for the condition variable
             * Will only be effective in the thread running the strand
             */
            void wait() {
                if (get_strand().running_in_this_thread()) {
                    suspended_.push_back(handler_t(ctx_));
                    ctx_.ca_();
                }
            }
            
            /**
             * Notify one handler in the waiting queue
             * The notification procedure will be invoked in the strand
             */
            void notify_one() {
                if (get_strand().running_in_this_thread()) {
                    if (suspended_.empty()) return;
                    get_strand().post(suspended_.front());
                    suspended_.pop_front();
                } else {
                    get_strand().post([this](){
                        if (suspended_.empty()) return;
                        get_strand().post(suspended_.front());
                        suspended_.pop_front();
                    });
                }
            }
            
            /**
             * Notify all handlers in the waiting queue
             * The notification procedure will be invoked in the strand
             */
            void notify_all() {
                if (get_strand().running_in_this_thread()) {
                    for (handler_t &h : suspended_) get_strand().post(h);
                    suspended_.clear();
                } else {
                    get_strand().post([this](){
                        for (handler_t &h : suspended_) get_strand().post(h);
                        suspended_.clear();
                    });
                }
            }
            
            strand &get_strand()
            { return ctx_.handler_.dispatcher_; }
            
            yield_context get_yield_context() const
            { return ctx_; }
            
        private:
            boost::asio::yield_context ctx_;
            std::deque<handler_t> suspended_;
        };
        
        struct condition_flag {
            explicit condition_flag(yield_context ctx)
            : b_()
            , cond_(ctx)
            {}
            
            void wait()
            { while (!b_) cond_.wait(); }
            
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
