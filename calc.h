//
//  calc.hpp
//
//  Created by Xu Chen on 13-2-23.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

#ifndef calc_h_included
#define calc_h_included

/*
 * Simple calculator
 */
template <typename Iterator>
struct calc_parser : boost::spirit::qi::grammar<Iterator, double(), boost::spirit::ascii::space_type> {
    calc_parser() : calc_parser::base_type(expression) {
        using boost::spirit::qi::double_;
        using boost::spirit::qi::_val;
        using boost::spirit::qi::_1;
        
        expression = term                       [_val=_1]
                     >> *( ('+' >> term         [_val=_val+_1])
                         | ('-' >> term         [_val=_val-_1])
                         )
        ;
        
        term = factor                           [_val=_1]
               >> *( ('*' >> factor             [_val=_val*_1])
                   | ('/' >> factor             [_val=_val/_1])
                   )
        ;
        
        factor = double_                        [_val=_1]
                 | '(' >> expression            [_val=_1] >> ')'
                 | ('+' >> factor               [_val=_1])
                 | ('-' >> factor               [_val=-_1])
        ;

        BOOST_SPIRIT_DEBUG_NODE(expression);
        BOOST_SPIRIT_DEBUG_NODE(term);
        BOOST_SPIRIT_DEBUG_NODE(factor);
    }
    
    boost::spirit::qi::rule<Iterator, double(), boost::spirit::ascii::space_type> expression;
    boost::spirit::qi::rule<Iterator, double(), boost::spirit::ascii::space_type> term;
    boost::spirit::qi::rule<Iterator, double(), boost::spirit::ascii::space_type> factor;
};

#endif  /* defined(calc_h_included) */
