#include "args_parse.hh"

#include <cstring>

#include "test_marcos.hh"

namespace ivanp { namespace args_parse {

// TODO: bug: treat only short options correctly

args_parse& args_parse::parse(int argc, char const * const * argv) {
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

bool args_parse::help(int argc, char const * const * argv,
                      std::string str, bool no_args_help,
                      std::ostream& os) const
{
  if (no_args_help && argc==1) {
    print_help(str,os);
    return true;
  } else if (str.size()) {

    for (int k=1; k<argc; ++k) {
      for (size_t i=0, j=str.find(','); ; ) {
        const unsigned n = j-i;
        const auto len = strlen(argv[k]);
        static bool need_help = false;

        switch (len) {
          case 0: break;
          case 1: break;
          case 2: need_help = (
            argv[k][0]=='-' && n==1 && str[i]==argv[k][1] );
            break;
          default: need_help = ( argv[k][0]=='-' && argv[k][1]=='-'
            && !strcmp(&str[i],argv[k]+2) );
        }

        if (need_help) {
          print_help(str,os);
          return true;
        }

        if (j==std::string::npos) break;
        j = str.find(',',i=j+1);
      }
    }

  }
  return false;
}

void args_parse::print_help(const std::string& str, std::ostream& os) const {
  for (auto& p : arg_descs) {
    arg_proxy_base* opt = nullptr;

    os << "    \e[1m";
    for (size_t i=0, j=p.first.find(','); ; ) {
      const size_t n = j-i;

      if (!opt) {
        opt = ( n==1 ? short_argmap.at(p.first[i])
                     : long_argmap.at(p.first.substr(i,n)) );
      }

      os << '-';
      if (n>1) os << '-';
      os << p.first.substr(i,n);

      if (j==std::string::npos) break;
      else os << ", ";
      j = str.find(',',i=j+1);
    }

    // print flags
    if (opt->flags & flags_t::required) os << " *";

    os << "\e[0m\n        " << p.second << "\n" << std::endl;
  }
}

}} // end namespace
