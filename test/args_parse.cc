#include <iostream>
#include <sstream>
#include <vector>
#include <array>
#include <map>

template <typename... TT> struct show_type;

#define ARGS_PARSE_USE_BOOST_LEXICAL_CAST
#include "args_parse.hh"

#include "test_class.hh"
#include "test_marcos.hh"

using std::cout;
using std::endl;
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
  long unsigned l = 0;
  double d;
  std::string str;
  std::vector<std::array<std::string,3>> v;
  std::array<int,2> a;
  std::tuple<std::string,size_t> t{{},0};
  // std::map<std::string,std::pair<double,double>> m;
  std::map<std::string,std::vector<std::string>> m;
  // std::vector<std::map<std::string,std::string>> vm;
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
      ("v,vec",&v,"vector")
      ("m,map",&m,"map",
        ap::no_default,
        [](decltype(m)* m, const std::string& str){
          const auto d = str.find(':');
          (*m)[{str.data(),d}].emplace_back(str.data()+d+1);
        })
      ("a,arr",&a,"array", std::forward_as_tuple(1,2))
      ("t,tup",&t,"tuple",
        // ap::no_default,
        std::forward_as_tuple("text",42)
        // [](decltype(t)* t, const std::string& str){
        //   std::get<0>(*t) = str;
        //   std::get<1>(*t) = str.size();
        // }
      )
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
        })
      .parse(argc,argv);
  } catch ( std::exception& e ) {
    std::cerr << "\033[31margs:\033[0m " << e.what() << std::endl;
    return 1;
  }

  test( i )
  test( l )
  test( d )
  test( str )
  for (auto ai : a) test( ai )
  test( std::get<0>(t) )
  test( std::get<1>(t) )
  test( f.s )
  test( str1 )
  cout << "\033[36mvector\033[0m\n";
  for (const auto& a : v) {
    for (const auto& s : a)
      cout << s << ' ';
    cout << endl;
  }
  cout << "\033[36mmap\033[0m\n";
  for (const auto& p : m) {
    cout << p.first << " :";
    for (const auto& s : p.second) cout <<' '<< s;
    cout << endl;
  }
  // for (const auto& m : vm) {
  //   for (const auto& p : m)
  //     cout << p.first << " : " << p.second << endl;
  // }

  return 0;
}
