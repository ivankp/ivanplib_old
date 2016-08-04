// Written by Ivan Pogrebnyak

#include <iostream>
#include <type_traits>
#include <vector>
#include <array>
#include <string>

#include "test_marcos.hh"
#include "extra_traits.hh"

struct foo { };
struct bar { };

DEFINE_IS_TEMPLATE_TRAIT(std::vector,vector);
DEFINE_IS_TEMPLATE_TRAIT(std::basic_string,basic_string);
DEFINE_IS_TYPE_TRAIT(std::string,string);
DEFINE_IS_TYPE_TRAIT(foo,foo);

#define L std::cout << std::endl;

int main(int argc, char const *argv[]) {

  using trait = ivanp::find_first_type< std::is_arithmetic,
    foo, std::array<double,3>, std::vector<int>, double, bar, int
  >;

  print_type( trait )
  print_type( trait::type )
  test( trait::value ) L

  test( is_vector<std::vector<int>>::value )
  test( is_vector<std::string>::value )
  test( (is_vector<std::array<int,3>>::value) )
  test( is_vector<int>::value ) L

  test( is_basic_string<std::vector<int>>::value )
  test( is_basic_string<std::string>::value )
  test( is_basic_string<std::basic_string<char>>::value )
  test( is_basic_string<std::basic_string<wchar_t>>::value ) L

  test( is_string<std::vector<int>>::value )
  test( is_string<std::basic_string<char>>::value )
  test( is_string<std::basic_string<wchar_t>>::value )
  test( is_string<std::string>::value )
  test( is_string<const std::string>::value )
  test( is_string<std::string&>::value )
  test( is_string<std::string&&>::value ) L

  test( is_foo<std::string>::value )
  test( is_foo<foo>::value )
  test( is_foo<bar>::value )

  return 0;
}
