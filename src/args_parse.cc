#include "args_parse.hh"

#include <cstring>

#include "test_marcos.hh"

namespace ivanp { namespace args_parse {

//args_parse& help(std::string opt, bool no_args_help) {
//  if (opt.size()==0) {
//    this->no_args_help = true;
//  } else {
//    help_opt = std::move(opt);
//    this->no_args_help = no_args_help;
//  }
//}

args_parse& args_parse::parse(int argc, char const * const * argv) {
//  if (no_args_help && argc==1) {
//    print_help();
//    return *this;
//  } else if (help_opt.size()) {

//    for (size_t i=0, j=help_opt.find(','); ; ) {
//      const size_t n = j-i;

//      for (int i=1; i<argc; ++i) {
//        if (!strcmp(argv[i])) {

//        }
//      }

//      if (j==std::string::npos) break;
//      j = help_opt.find(',',i=j+1);
//    }

//  }

  arg_proxy_base* opt = nullptr;
  for (int i=1; i<argc; ++i) {
    const auto len = strlen(argv[i]);
    if (len>0 && argv[i][0]=='-') { // option
      if (len>1 && argv[i][1]=='-') { // long option
        auto opt_it = long_argmap.find(argv[i]+2);
        if (opt_it==long_argmap.end()) throw std::runtime_error(
          std::string("unknown option: \e[1m").append(argv[i],len)+"\e[0m");
        opt = opt_it->second;
      } else if (len==2) { // short option
        auto opt_it = short_argmap.find(argv[i][1]);
        if (opt_it==short_argmap.end()) throw std::runtime_error(
          std::string("unknown option: \e[1m-")+argv[i][1]+"\e[0m");
        opt = opt_it->second;
      } else throw std::runtime_error(
        std::string("unexpected program argument: \e[1m").append(argv[i],len)
        + "\e[0m");
    } else { // value
      if ( !(opt->flags & multiple) && opt->count ) throw std::runtime_error(
          std::string("unique option \e[1m").append(argv[i],len)
          + "\e[0m passed more than once");
      ++opt->count;
      opt->parse(argv[i],len);
    }
  }

  for (arg_proxy_base* opt : arg_proxies) {
    if (opt->count==0 && (opt->flags & flags_t::required))
      throw std::runtime_error(
        "required option \e[1m"+(opt->opt->first)+"\e[0m not passed\n"
        "option description: " + opt->opt->second);
  }
  for (arg_proxy_base* opt : arg_proxies) {
    if (opt->count==0) opt->assign_default();
  }

  return *this;
}

}} // end namespace
