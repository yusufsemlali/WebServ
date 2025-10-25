#ifndef VERBOSE_LOGGING_HPP
#define VERBOSE_LOGGING_HPP

#include <iostream>

// Verbose logging macros
#ifdef VERBOSE_LOGGING
    #define VLOG(x) std::cout << x << std::endl
    #define VLOG_NO_NL(x) std::cout << x
#else
    #define VLOG(x) do {} while(0)
    #define VLOG_NO_NL(x) do {} while(0)
#endif

#endif // VERBOSE_LOGGING_HPP
