// Written by Ivan Pogrebnyak

#ifndef IVANP_ARGS_PARSE_HH
#define IVANP_ARGS_PARSE_HH

#include <string>
#include <tuple>
#include <unordered_map>
#include <memory>

#include "extra_traits.hh"

namespace ivanp { namespace args_parse {

  enum flags_t {
    none = 0,
    required = 1
  };

  namespace detail {
    // using std_string = std::string;
    // DEFINE_IS_TYPE_TRAIT(std_string)
    DEFINE_IS_TYPE_TRAIT(flags_t)
  }

  class args_parse {

    enum iflags_t {
      none = 0,
      has_default = 1,
      non_unique = 2
    };

    // arg_proxy_base -----------------------------------------------
    struct arg_proxy_base {
      std::string opt, desc;
      int count;
      flags_t flags;
      iflags_t iflags;

      virtual void parse(void* ptr, const std::string& str) const =0;
      virtual void assign_default(void* ptr) const { }
      virtual ~arg_proxy_base() { };
    };

    // arg_with_default ---------------------------------------------
    template <typename T, typename... Args> class arg_default {
      std::tuple<Args...> args;

      // http://stackoverflow.com/a/7858971/2640636
      template <int ...> struct seq { };
      template <int N, int ...S> struct gens : gens<N-1, N-1, S...> { };
      template <int ...S> struct gens<0, S...> { typedef seq<S...> type; };
      template <int N> using gens_t = typename gens<N>::type;

      template <int ...S>
      inline typename std::enable_if<(sizeof...(S)!=1)>::type
      assign_default_impl(T& x, seq<S...>) const {
        x = T(std::get<S>(args)...);
      }
      template <int ...S>
      inline typename std::enable_if<(sizeof...(S)==1)>::type
      assign_default_impl(T& x, seq<S...>) const {
        x = std::get<0>(args);
      }
      // TODO: provide better alternatives for assignment
      // http://en.cppreference.com/w/cpp/header/type_traits
    public:
      inline void assign_default(void* ptr) const {
        assign_default_impl( *reinterpret_cast<T*>(ptr),
                             gens_t<sizeof...(Args)>() );
      }
    };
    DEFINE_IS_TEMPLATE_TRAIT(arg_default)

    // arg_with_parser ----------------------------------------------
    template <typename T, typename Parser> struct arg_parser {
      Parser parser;
      inline void parse(void* ptr, const std::string& str) {
        parser(reinterpret_cast<T*>(ptr),str);
      }
    };
    DEFINE_IS_TEMPLATE_TRAIT(arg_parser)

    // arg_proxy ----------------------------------------------------
    // template <bool HasDefault, bool HasParser, typename... TT>
    // struct arg_proxy { };
    //
    // template <> struct arg_proxy<false,false>: public arg_proxy_base { };
    //
    // template <typename Default, typename Parser>
    // struct arg_proxy<true,true,Default,Parser>
    // : public arg_proxy_base, private Default, private Parser {
    //   virtual void assign_default(void* ptr) const {
    //     Default::assign_default(ptr);
    //   }
    //   virtual void parse(void* ptr, const std::string& str) {
    //     Parser::parse(ptr,str);
    //   }
    // };
    //
    // template <typename Default>
    // struct arg_proxy<true,false,Default>
    // : public arg_proxy_base, private Default {
    //   virtual void assign_default(void* ptr) const {
    //     Default::assign_default(ptr);
    //   }
    // };
    //
    // template <typename Parser>
    // struct arg_proxy<false,true,Parser>
    // : public arg_proxy_base, private Parser {
    //   virtual void parse(void* ptr, const std::string& str) {
    //     Parser::parse(ptr,str);
    //   }
    // };

    std::unordered_map<void*,std::unique_ptr<arg_proxy_base>> argmap;
    // value needs to be a pointer for polymorphism to work
    // TODO: make sure this is the best way

  public:
    args_parse() =default;
    args_parse& parse(int argc, char const **argv);
    // TODO: implement parsing function in .cc

    template <typename T, typename S1, typename S2, typename... Args>
    typename std::enable_if<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value,
    args_parse >::type&
    operator()( S1&& option, T* x, S2&& description, Args&&... args) {
      arg_proxy_base *proxy = nullptr;

      print_type(T)
      print_pack_types<Args...>();

      // find_first_type<is_arg_default, TT...>
      // find_first_type<is_arg_parser, TT...>


      argmap.emplace( std::piecewise_construct,
        std::forward_as_tuple(reinterpret_cast<void*>(x)),
        std::forward_as_tuple(proxy)
      );
      return *this;
    }

  }; // end args_parse class

}} // end namespace

#endif
