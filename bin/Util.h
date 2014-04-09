#ifndef UTIL_H
#define UTIL_H
#include<sstream>
template<class out_type, class in_value>
out_type convert(const in_value &t)
{
    stringstream stream;
    stream<<t;
    out_type result;
    stream>>result;
    return result;
}
#endif
