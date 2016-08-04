// Written by Ivan Pogrebnyak

#ifndef IVANP_TEST_MACROS_HH
#define IVANP_TEST_MACROS_HH

#define test(var) \
  std::cout << "\033[36m" << #var << "\033[0m = " << var << std::endl;

#include <boost/type_index.hpp>

#define print_type(type) \
  std::cout << "\033[36m" << #type << "\033[0m = " \
            << boost::typeindex::type_id<type>().pretty_name() \
            << std::endl;

#define print_typeof(var) \
  std::cout << "\033[36mdecltype(" << #var << ")\033[0m = " \
            << boost::typeindex::type_id<decltype( var )>().pretty_name() \
            << std::endl;

template <typename T> void print_pack_types() {
  std::cout << boost::typeindex::type_id<T>().pretty_name() << std::endl;
}
template <typename T, typename... TT>
typename std::enable_if<sizeof...(TT)>::type print_pack_types() {
  std::cout << boost::typeindex::type_id<T>().pretty_name() << std::endl;
  print_pack_types<TT...>();
}

#endif
