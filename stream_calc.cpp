//
//  stream_calc.cpp
//  coroserver
//
//  Created by Xu Chen on 13-2-24.
//  Copyright (c) 2013 Xu Chen. All rights reserved.
//

#include <iomanip>
#include <boost/algorithm/string.hpp>
#include "calc.h"
#include "stream_calc.h"

void stream_calc(std::istream &sin, std::ostream &sout) {
    sout << "Hello!" << std::endl;
    
    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    
    calc_parser<iterator_type> calc;
    std::string line;
    sout << std::fixed;
    while (std::getline(sin, line)) {
        boost::trim(line);
        if(line=="quit") break;
        iterator_type first=line.cbegin();
        iterator_type last=line.cend();
        double v;
        bool r=phrase_parse(first, last, calc, space, v);
        if (r && first==last) {
            sout << v << std::endl;
        } else {
            sout << "Parse error" << std::endl;
        }
    }
    sout << "Bye!" << std::endl;
}
