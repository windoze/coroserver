//
//  stream_calc.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <iomanip>
#include <boost/algorithm/string/trim.hpp>
#include "calculator.h"

namespace calculator {
    namespace details {
        typedef double value_t;
        
        /*
         * Calculator of value_t
         */
        template <typename Iterator>
        struct calc_parser : boost::spirit::qi::grammar<Iterator, value_t(), boost::spirit::ascii::space_type> {
            calc_parser() : calc_parser::base_type(expression) {
                using boost::spirit::qi::double_;
                using boost::spirit::qi::_val;
                using boost::spirit::qi::_1;
                
                expression = term      [_val=_1]
                >> *( ('+' >> term     [_val=_val+_1])
                    | ('-' >> term     [_val=_val-_1])
                    )
                ;
                
                term = factor          [_val=_1]
                >> *( ('*' >> factor   [_val=_val*_1])
                    | ('/' >> factor   [_val=_val/_1])
                    )
                ;
                
                factor = double_       [_val=_1]
                | '(' >> expression    [_val=_1] >> ')'
                | ('+' >> factor       [_val=_1])
                | ('-' >> factor       [_val=-_1])
                ;
                
                BOOST_SPIRIT_DEBUG_NODE(expression);
                BOOST_SPIRIT_DEBUG_NODE(term);
                BOOST_SPIRIT_DEBUG_NODE(factor);
            }
            
            boost::spirit::qi::rule<Iterator, value_t(), boost::spirit::ascii::space_type> expression;
            boost::spirit::qi::rule<Iterator, value_t(), boost::spirit::ascii::space_type> term;
            boost::spirit::qi::rule<Iterator, value_t(), boost::spirit::ascii::space_type> factor;
        };
    }   // End of namespace details
    
    void protocol_handler(net::async_tcp_stream &s) {
        s << "Hello!" << std::endl;
        
        using boost::spirit::ascii::space;
        typedef std::string::const_iterator iterator_type;
        
        details::calc_parser<iterator_type> calc;
        std::string line;
        while (std::getline(s, line)) {
            boost::trim(line);
            if(line=="quit") break;
            iterator_type first=line.cbegin();
            iterator_type last=line.cend();
            details::value_t v;
            bool r=phrase_parse(first, last, calc, space, v);
            if (r && first==last) {
                s << v << std::endl;
            } else {
                s << "Parse error" << std::endl;
            }
        }
        s << "Bye!" << std::endl;
    }
}   // End of namespace calculator