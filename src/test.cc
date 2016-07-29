#include <iostream>
#include <type_traits>
#include <vector>
#include <array>

template<typename T>
struct is_vector: std::false_type { };

template<typename T, typename Alloc>
struct is_vector<std::vector<T,Alloc>>: std::true_type { };

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::endl;

int main(int argc, char const *argv[]) {

  test( (is_vector<std::vector<int>>::value) )
  test( (is_vector<std::array<int,5>>::value) )

  return 0;
}
