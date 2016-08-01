// Written by Ivan Pogrebnyak

#ifndef IVANP_TEST_MACROS_HH
#define IVANP_TEST_MACROS_HH

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

#include <boost/type_index.hpp>

#define print_type(var) \
  std::cout <<"\033[36m"<< #var <<" type\033[0m"<< " = " \
            << boost::typeindex::type_id<decltype( var )>().pretty_name() \
            << std::endl;

#endif
