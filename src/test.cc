#include <iostream>

#include <boost/type_index.hpp>

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

#define print_type(var) \
  std::cout <<"\033[36m"<< #var <<" type\033[0m"<< " = " \
            << boost::typeindex::type_id<decltype( var )>().pretty_name() \
            << std::endl;

#include "sumsq.hh"

using std::cout;
using std::endl;

using namespace goop;

int main(int argc, char const *argv[]) {

  std::array<std::array<int,3>,3> aa1 {
    1,2,3, 4,5,6, 7,8,9
  };
  print_type( aa1 )

  // print_type( quad_sum(aa1,2.f) )
  // for (auto x : quad_sum(aa1,2.f)) cout << ' ' << x;

  return 0;
}
