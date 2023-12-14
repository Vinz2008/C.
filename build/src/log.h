#include <iostream>

#ifndef _LOG_HEADER_
#define _LOG_HEADER_

extern bool silent_mode;

struct Log {
    template< class T >
        Log& operator<<(const T& val){
            if (!silent_mode){
                std::cout << val;
            }
            return *this;
        }
};

#endif