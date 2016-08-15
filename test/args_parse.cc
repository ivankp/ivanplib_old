#include <iostream>
#include <sstream>
#include <vector>
#include <array>

#include "test_class.hh"
#include "test_marcos.hh"

template <typename... TT> struct show_type;

#include "args_parse.hh"

namespace ap = ivanp::args_parse;

int main(int argc, const char* argv[])
{
  std::string str1("Hello world!");

  // auto tup1 = std::forward_as_tuple(foo(str1));
  // auto tup1 = std::make_tuple(foo(str1));
  // std::tuple<std::string&&> tup1 = std::make_tuple(str1);
  // std::tuple<foo&&> tup1 = std::make_tuple(foo(str1));
  // const auto& tup2ref = tup2;
  // std::tuple<foo&&> tup2{
  //   // std::string(str1)
  //   // std::move(std::get<0>(tup1))
  // };
  // std::tuple<foo> tup3{
  //   str1
  //   // std::move(std::get<0>(tup1))
  // };

  int i;
  long unsigned l;
  double d;
  std::string str;
  std::vector<char> v;
  std::array<int,2> a;
  foo f;

  try {
    ap::args_parse()
      ("l,long",&l,"long"/*,ap::required*/)
      ("d,double",&d,"double", 5.5)
      ("s,string",&str,"string", str1)
      ("i,int",&i,"int", -1,
        [](int* i, const std::string& str){
          (*i) = str.size();
        }/*,ap::required*/)
      ("v,vec",&v,"vector", std::forward_as_tuple(str1.begin()+1,str1.begin()+3))
      ("a,arr",&a,"array", std::forward_as_tuple(1,2))
      ("f,foo",&f,"test class",
        // ap::no_default,
        'x',
        // "default value",
        // str1,
        // foo("default value"),
        // foo(str1),
        // std::forward_as_tuple(3,'x'),
        // std::forward_as_tuple("default value"),
        // std::forward_as_tuple(str1),
        // std::forward_as_tuple(foo("default")),
        // std::make_tuple("default value"),
        // std::make_tuple(str1),
        // std::make_tuple(foo("default")),
        // std::tuple<foo>{"default"},
        // std::move(tup1),
        // std::move(tup3),
        // tup3,
        // std::tie(str1),
        // std::get<0>(tup3),
        // std::tie(std::get<0>(tup3)),
        [](foo* f, const std::string& str){
          *f = str;
          // f->s = str;
        })
      .parse(argc,argv);
  } catch ( std::exception& e ) {
    std::cerr << "args: " << e.what() << std::endl;
    return 1;
  }

  test( i )
  test( l )
  test( d )
  test( str )
  for (auto vi : v) test( vi )
  for (auto ai : a) test( ai )
  test( f.s )
  test( str1 )

  return 0;
}
