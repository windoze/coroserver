//
//  server.h
//  coroserver
//
//  Created by Windoze on 13-2-24.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef server_h_included
#define server_h_included

#include <functional>
#include <string>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include "logging.h"
#include "async_stream.h"

namespace net {
    typedef std::function<bool(boost::asio::io_service&)> initialization_handler_t;
    typedef std::function<void(boost::asio::io_service&)> finalization_handler_t;
    typedef std::string endpoint_t;
    typedef std::function<void(async_tcp_stream&)> protocol_handler_t;
    typedef std::pair<endpoint_t, protocol_handler_t> sap_desc_t;
    typedef std::vector<sap_desc_t> sap_desc_list_t;
    
    /**
     * Stream-oriented socket server
     */
    class server {
    public:
        /**
         * Constructor
         *
         * @param sap_desc_list protocols and their service access points (addresses and ports)
         * @param thread_pool_size number of threads that run simultaneously to process client connections
         */
        server(const sap_desc_list_t &sap_desc_list,
               std::size_t thread_pool_size);
        
        /**
         * Constructor
         *
         * @param sap_desc_list protocols and their service access points (addresses and ports)
         * @param initialization_handler called before setting up server
         * @param finalization_handler called before server destruction
         * @param thread_pool_size number of threads that run simultaneously to process client connections
         */
        server(const sap_desc_list_t &sap_desc_list,
               const initialization_handler_t &initialization_handler,
               const finalization_handler_t &finalization_handler,
               std::size_t thread_pool_size);
        
        // Non-copyable
        server(const server&) = delete;
        server& operator=(const server&) = delete;
        
        ~server();
        
        /**
         * Start server
         */
        inline void operator()()
        { run(); }
        
        /**
         * Initialization result
         */
        inline bool initialized() const
        { return init_state_; }
        
    private:
        void listen(const sap_desc_t &sd);
        void open();
        void close();
        void run();
        void handle_connect(boost::asio::ip::tcp::socket &&socket, const protocol_handler_t &handler);
        
        logging::logger_t logger_;
        std::size_t thread_pool_size_;
        boost::asio::io_service io_service_;
        boost::asio::signal_set signals_;
        initialization_handler_t initialization_handler_;
        finalization_handler_t finalization_handler_;
        bool init_state_;
        typedef std::pair<boost::asio::ip::tcp::acceptor, protocol_handler_t> sap_t;
        typedef std::shared_ptr<sap_t> sap_ptr;
        typedef std::vector<sap_ptr> sap_list_t;
        sap_list_t saps_;
    };
}   // End of namespace net

#endif /* defined(server_h_included) */
