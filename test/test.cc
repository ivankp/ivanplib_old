// Written by Ivan Pogrebnyak

#include <iostream>
#include <type_traits>
#include <vector>
#include <array>
#include "test_marcos.hh"

template<typename T>
struct is_vector: std::false_type { };

template<typename T, typename Alloc>
struct is_vector<std::vector<T,Alloc>>: std::true_type { };

using std::cout;
using std::endl;

int main(int argc, char const *argv[]) {

  test( (is_vector<std::vector<int>>::value) )
  test( (is_vector<std::array<int,5>>::value) )

  return 0;
}
