#include "args_parse.hh"

#include <stdexcept>

#include "test_marcos.hh"

using namespace std;

namespace ivanp { namespace args_parse {

// TODO: assign default values
// TODO: add syntax for short and long options

args_parse& args_parse::parse(int argc, char const **argv) {
  for (const auto& opt : argmap) {
    string short_opt, long_opt;
    const string& opt_str = opt.second->opt;

    if (opt_str.size()==1) {
      short_opt = string("-")+opt_str[0];
    } else if (opt_str.size()>1) {
      if (opt_str[1]==',') {
        short_opt = string("-")+opt_str[0];
        long_opt = opt_str.substr(2);
      } else {
        long_opt = opt_str;
      }
    } else throw runtime_error("blank option string");

    for (int argi=1; argi<argc; ++argi) {
      if ( (short_opt.size() && short_opt==argv[argi])
        || ( long_opt.size() &&  long_opt==argv[argi]) ) {
        if ( !(opt.second->iflags & non_unique)
          && opt.second->count ) throw runtime_error(
            "unique argument "+opt_str+" passed more then once");
        ++opt.second->count;
        opt.second->parse(opt.first,argv[++argi]);
      }
    }

    if (opt.second->count==0) {
      if (opt.second->flags & flags_t::required)
        throw runtime_error("required argument "+opt_str+" not passed");
      if (opt.second->iflags & has_default)
        opt.second->assign_default(opt.first);
    }
  }

  return *this;
}

}} // end namespace
