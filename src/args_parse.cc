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
      if (!opt) {
        if (!positional.size()) throw std::runtime_error(
          std::string("argument without option: ").append(argv[i],len)
        );
        opt = positional.front();
        if (opt->pos) positional.pop();
      }
      const bool is_unique = !(opt->flags & multiple);
      if ( is_unique && opt->count ) throw std::runtime_error(
          "unique option \e[1m" + opt->opt->first
          + "\e[0m passed more than once");
      ++opt->count;
      opt->parse(argv[i],len);
      if (is_unique) opt = nullptr;
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

args_parse& args_parse::pos(const char* str, unsigned i) {
  return pos(str, strlen(str), i);
}

args_parse& args_parse::pos(const char* str, size_t n, unsigned i) {
  positional.push( n==1 ? short_argmap.at(str[0]) : long_argmap.at({str,n}) );
  auto* opt = positional.back();
  opt->flags |= flags_t::positional;
  if (i) opt->pos = positional.size();
  for (unsigned j=1; j<i; ++j) positional.push(opt);
  return *this;
}

bool args_parse::help(int argc, char const * const * argv,
                      const std::string& str, bool no_args_help,
                      std::ostream& os) const
{
  if (no_args_help && argc==1) {
    print_help(str,os);
    return true;
  } else if (str.size()) {

    for (int k=1; k<argc; ++k) {
      const auto len = strlen(argv[k]);

      for (size_t i=0, j=str.find(','); ; ) {
        if (j==std::string::npos) j = str.size();
        const unsigned n = j-i;
        static bool need_help = false;

        switch (len) {
          case 0: break;
          case 1: break;
          case 2: need_help = (
            argv[k][0]=='-' && n==1 && str[i]==argv[k][1] );
            break;
          default: need_help = ( argv[k][0]=='-' && argv[k][1]=='-' && n>1
            && !strcmp(&str[i],argv[k]+2) );
        }

        if (need_help) {
          print_help(str,os);
          return true;
        }

        if (j==str.size()) break;
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
      if (j==std::string::npos) j = p.first.size();
      const size_t n = j-i;

      if (!opt) {
        opt = ( n==1 ? short_argmap.at(p.first[i])
                     : long_argmap.at(p.first.substr(i,n)) );
      }

      os << '-';
      if (n>1) os << '-';
      os << p.first.substr(i,n);

      if (j==p.first.size()) break;
      else os << ", ";
      j = str.find(',',i=j+1);
    }

    // print flags
    if (opt->flags & flags_t::required) os << " *";

    if (opt->flags & flags_t::positional) {
      os << " pos:" << opt->pos;
    }

    os << "\e[0m\n        " << p.second << "\n" << std::endl;
  }
}

}} // end namespace
