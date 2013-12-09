//
//  logging.h
//  coroserver
//
//  Created by Chen Xu on 13-12-9.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#ifndef __coroserver__logging__
#define __coroserver__logging__

#include <exception>
#include <string>
#include <iostream>
#include <boost/log/common.hpp>
#include <boost/log/attributes/function.hpp>

namespace logging {
    enum severity_level
    {
        trace,
        debug,
        info,
        warning,
        error,
        fatal
    };
    
    typedef boost::log::sources::severity_logger<severity_level> logger_t;
    
    void init();
    void init(std::istream &settings);
}   // End of namespace logging

#define LOG_TRACE(logger)   BOOST_LOG_SEV(logger, logging::trace)
#define LOG_DEBUG(logger)   BOOST_LOG_SEV(logger, logging::debug)
#define LOG_INFO(logger)    BOOST_LOG_SEV(logger, logging::info)
#define LOG_WARNING(logger) BOOST_LOG_SEV(logger, logging::warning)
#define LOG_ERROR(logger)   BOOST_LOG_SEV(logger, logging::error)
#define LOG_FATAL(logger)   BOOST_LOG_SEV(logger, logging::fatal)

#endif /* defined(__coroserver__logging__) */
