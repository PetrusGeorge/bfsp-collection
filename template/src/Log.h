#ifndef LOG_H
#define LOG_H

#include <iostream>

#ifdef NDEBUG
    #define DEBUG  if (false) std::cout
    #define DEBUG_EXTRA  if (false) std::cout
#else
    #define DEBUG std::cout
    #define DEBUG_EXTRA std::cout << "[" << __FILE__ << ":" << __LINE__ << "] "
#endif

#define VERBOSE(verbosity)  if (verbosity) std::cout

#endif
