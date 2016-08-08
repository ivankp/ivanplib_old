#include <iostream>
#include <sstream>
#include <vector>
#include <array>

#include "test_marcos.hh"

template <typename... TT> struct show_type;

#include "args_parse.hh"

struct foo {
  static unsigned n;
  unsigned i;

  std::string s;
  foo() { std::cout << "default " << (i=(n++)) << std::endl; }
  foo(const foo& f): s(f.s) { std::cout << "copy " << (i=(n++)) << std::endl; }
  foo(foo&& f): s(std::move(f.s)) { std::cout << "move " << (i=(n++)) << std::endl; }
  foo& operator=(const foo& f) {
    s = f.s;
    std::cout << "copy assign " << i << std::endl;
    return *this;
  }
  foo& operator=(foo&& f) {
    s = std::move(f.s);
    std::cout << "move assign " << i << std::endl;
    return *this;
  }
  template <typename T> foo(T&& x): s(std::forward<T>(x)) {
    std::cout << "construct " << (i=(n++)) << std::endl;
  }
  template <typename T>
  typename std::enable_if<!std::is_same<
    typename std::remove_cv<T>::type,foo>::value,foo>::type&
  operator=(T&& x) {
    s = std::forward<T>(x);
    std::cout << "assign " << i << std::endl;
    return *this;
  }
  ~foo() { std::cout << "delete " << i << std::endl; }
};
unsigned foo::n = 0;

using namespace std;
namespace ap = ivanp::args_parse;

int main(int argc, const char* argv[])
{
  std::string str1("Hello world!");

  // auto tup1 = std::forward_as_tuple(foo(str1));
  // const auto& tup2ref = tup2;
  // std::tuple<foo&&> tup2{
  //   std::move(tup1)
  //   // std::move(std::get<0>(tup1))
  // };
  std::tuple<foo> tup3{
    str1
    // std::move(std::get<0>(tup1))
  };

  // test(get<0>(tup1).s)
  // test(get<0>(tup2).s)
  // test(get<0>(tup3).s)

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
      ("v,vec",&v,"vector", std::forward_as_tuple(str1.begin(),str1.begin()+1))
      ("a,arr",&a,"array",  std::forward_as_tuple(1,2))
      ("f,foo",&f,"test class",
        // "default",
        // foo("default"),
        // std::forward_as_tuple(foo("default")),
        // std::forward_as_tuple("default"),
        // std::move(tup3),
        tup3,
        [](foo* f, const std::string& str){
          // *f = str;
          f->s = str;
        })
      .parse(argc,argv);
  } catch ( std::exception& e ) {
    cerr << "args: " << e.what() << endl;
    return 1;
  }

  test( i )
  test( l )
  test( d )
  test( str )
  for (auto vi : v) test( vi )
  for (auto ai : a) test( ai )
  test( f.s )

  return 0;
}
