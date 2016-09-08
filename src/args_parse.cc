#include "args_parse.hh"

#include <cstring>

#include "test_marcos.hh"

namespace ivanp { namespace args_parse {

args_parse& args_parse::parse(int argc, char const * const * argv) {
  arg_proxy_base* opt = nullptr;
  for (int i=1; i<argc; ++i) {
    const auto len = strlen(argv[i]);
    if (len>0 && argv[i][0]=='-') { // option
      if (len>1 && argv[i][1]=='-') { // long option
        auto opt_it = long_argmap.find(argv[i]+2);
        if (opt_it==long_argmap.end()) throw std::runtime_error(
          std::string("unknown option: ").append(argv[i],len));
        opt = opt_it->second;
      } else if (len==2) { // short option
        auto opt_it = short_argmap.find(argv[i][1]);
        if (opt_it==short_argmap.end()) throw std::runtime_error(
          std::string("unknown option: -")+argv[i][1]);
        opt = opt_it->second;
      } else throw std::runtime_error(
        std::string("unexpected program argument: ").append(argv[i],len));
    } else { // value
      if ( !(opt->flags & multiple) && opt->count ) throw std::runtime_error(
          std::string("unique argument ").append(argv[i],len)
          + " passed more than once");
      ++opt->count;
      opt->parse(argv[i],len);
    }
  }

  // TODO: let proxies know about their strings

  for (arg_proxy_base* opt : arg_proxies) {
    if (opt->count==0 && (opt->flags & flags_t::required))
      throw std::runtime_error("required argument ... not passed");
  }
  for (arg_proxy_base* opt : arg_proxies) {
    if (opt->count==0) opt->assign_default();
  }

  return *this;
}

}} // end namespace
