#include <iostream>
#include <sstream>
#include <tuple>

#include "test_marcos.hh"

#include "args_parse.hh"

using namespace std;
namespace ap = ivanp::args_parse;

int main(int argc, const char* argv[])
{

  int i;
  unsigned u;
  long l;
  double d;

  try {
    ap::args_parse()
      ("i,int",&i,"integer",ap::required)
      ("u,unsigned",&u,"unsigned",
        std::forward_as_tuple(42,"towel",5.),
        [](unsigned* u, const std::string& str){
          (*u) = str.size();
        });

  } catch ( std::exception& e ) {
    cerr << "args: " << e.what() << endl;
    return 1;
  }

  return 0;
}
