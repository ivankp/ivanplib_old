#include <iostream>
#include <sstream>
#include <vector>
#include <array>

#include "test_marcos.hh"

template <typename... TT> struct show_type;

#include "args_parse.hh"

using namespace std;
namespace ap = ivanp::args_parse;

int main(int argc, const char* argv[])
{
  std::string str1("Hello world!");

  // auto tup1 = std::forward_as_tuple(std::string("hello"));
  // // const auto& tup2ref = tup2;
  // std::tuple<std::string&&> tup2{
  //   std::move(tup1)
  //   // std::move(std::get<0>(tup1))
  // };
  // std::tuple<std::string> tup3{
  //   "world"
  //   // std::move(std::get<0>(tup1))
  // };
  //
  // test(get<0>(tup1))
  // test(get<0>(tup2))
  // test(get<0>(tup3))

  int i;
  long unsigned l;
  double d;
  float f;
  std::string str;
  std::vector<char> v;
  std::array<int,2> a;

  try {
    ap::args_parse()
      ("l,long",&l,"long"/*,ap::required*/)
      ("d,double",&d,"double", 5.5)
      ("f,float",&f,"float", 1.3)
      ("s,string",&str,"string",
        str
        // std::forward_as_tuple(str,5)
        // tup1
        // std::move(tup2)
        // tup3
        // "hello"
      )
      ("i,int",&i,"int", -1,
        [](int* i, const std::string& str){
          (*i) = str.size();
        }/*,ap::required*/)
      ("v,vec",&v,"vector", std::forward_as_tuple(str1.begin(),str1.begin()+2))
      ("a,arr",&a,"array",  std::forward_as_tuple(1,2))
      .parse(argc,argv);
  } catch ( std::exception& e ) {
    cerr << "args: " << e.what() << endl;
    return 1;
  }

  test( i )
  test( l )
  test( d )
  test( f )
  test( str )
  for (auto vi : v) test( vi )
  for (auto ai : a) test( ai )

  return 0;
}
