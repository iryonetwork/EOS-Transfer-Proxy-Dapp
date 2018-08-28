#pragma once
#include <boost/algorithm/string/trim.hpp>
#include <string>

std::string name_to_string(uint64_t acct_int) 
{
    static constexpr const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";
    std::string str(13,'.');
    uint64_t tmp = acct_int;

    for( uint32_t i = 0; i <= 12; ++i ) 
    {
        char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
        str[12-i] = c;
        tmp >>= (i == 0 ? 4 : 5);
    }

    boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
    return str;
}