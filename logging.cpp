//
//  logging.cpp
//  coroserver
//
//  Created by Chen Xu on 13-12-9.
//  Copyright (c) 2013 0d0a.com. All rights reserved.
//

#include <sstream>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/expressions.hpp>
#include "logging.h"

namespace boost {

    template<>
    logging::severity_level lexical_cast<logging::severity_level >(const std::string &s) {
        return static_cast<logging::severity_level>(lexical_cast<int>(s));
    }
}   // End of namespace boost

namespace logging {
    std::ostream& operator<<(std::ostream& os, severity_level l) {
        static const char* levels[] = {
            "TRACE",
            "DEBUG",
            "INFO",
            "WARNING",
            "ERROR",
            "FATAL"
        };
        
        if (static_cast<size_t>(l) < sizeof(levels) / sizeof(*levels)) {
            os << levels[l];
        } else {
            os << static_cast<int>(l);
        }
        
        return os;
    }

    void init() {
        std::stringstream s;
        s << "[Core]" << std::endl;
        s << "Filter=\"%Severity% >= 2\"" << std::endl;
        s << "[Sinks.Server]" << std::endl;
        s << "Destination=Console" << std::endl;
        s << "Filter=\"%Module% = \\\"Server\\\"\"" << std::endl;
        s << "Format=\"[%TimeStamp%][%Severity%][%Module%] %Message%\"" << std::endl;
        s << "[Sinks.HTTP]" << std::endl;
        s << "Destination=Console" << std::endl;
        s << "Filter=\"%Module% = \\\"HTTP\\\"\"" << std::endl;
        s << "Format=\"[%TimeStamp%][%Severity%][%Module%][CODE=%Code%] %Method% %URL% %Message%\"" << std::endl;
        init(s);
    }
    
    void init(std::istream &settings) {
        boost::log::add_common_attributes();
        boost::log::register_simple_formatter_factory<severity_level, char>("Severity");
        boost::log::register_simple_filter_factory<severity_level, char>("Severity");
        boost::log::register_simple_formatter_factory<std::string, char>("Module");
        boost::log::register_simple_formatter_factory<std::string, char>("Module");
        boost::log::register_simple_formatter_factory<std::string, char>("Remote");
        boost::log::register_simple_formatter_factory<std::string, char>("Local");
        boost::log::register_simple_formatter_factory<std::string, char>("Method");
        boost::log::register_simple_formatter_factory<std::string, char>("URL");
        //boost::log::register_simple_formatter_factory<int, char>("Code");
        boost::log::register_simple_filter_factory<int, char>("Code");
        boost::log::init_from_stream(settings);
    }
}   // End of namespace logging