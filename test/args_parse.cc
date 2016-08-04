#include <iostream>
#include <sstream>
#include <vector>

#include "test_marcos.hh"

#include "args_parse.hh"

using namespace std;
namespace ap = ivanp::args_parse;

int main(int argc, const char* argv[])
{
  std::string str1("Hello world!");

  int i;
  unsigned u;
  long l;
  double d;
  float f;
  std::string str;
  std::vector<int> v;

  try {
    ap::args_parse()
      ("l,long",&l,"long",ap::required)
      ("d,double",&d,"double", std::forward_as_tuple(5.0))
      ("f,float",&f,"float", 3.5)
      ("u,unsigned",&u,"unsigned",
        [](unsigned* u, const std::string& str){
          (*u) = str.size();
        },ap::required)
      ("s,string",&str,"string", std::forward_as_tuple(str,5))
      ("i,int",&i,"int", -1,
        [](int* i, const std::string& str){
          (*i) = str.size();
        })
      ("v,vec",&v,"vector", std::forward_as_tuple(1,2,3,4,5))
      .parse(argc,argv);
  } catch ( std::exception& e ) {
    cerr << "args: " << e.what() << endl;
    return 1;
  }

  test( i )
  test( u )
  test( l )
  test( d )
  test( f )
  test( str )
  for (auto vi : v) test( vi )

  return 0;
}
