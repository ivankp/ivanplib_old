// Written by Ivan Pogrebnyak

#ifndef IVANP_ARGS_PARSE_HH
#define IVANP_ARGS_PARSE_HH

#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <memory>

#include <sstream>

// #include <boost/lexical_cast.hpp>

#include "extra_traits.hh"
#include "expression_traits.hh"

namespace ivanp { namespace args_parse {

  enum flags_t {
    none = 0,
    required = 1
  };

  class args_parse {

    enum iflags_t {
      none = 0,
      has_default = 1,
      non_unique = 2
    };

    // arg_proxy ----------------------------------------------------
    struct arg_proxy_base {
      std::string opt, desc;
      flags_t flags;
      int count;
      iflags_t iflags;

      template <typename S1, typename S2>
      arg_proxy_base(S1&& opt, S2&& desc, flags_t flags)
      : opt(std::forward<S1>(opt)), desc(std::forward<S2>(desc)),
        flags(flags), count(0) { }

      virtual void parse(void* ptr, const std::string& str) const =0;
      virtual void assign_default(void* ptr) const { }
      virtual ~arg_proxy_base() { } // TODO: figure out if necessary
    };

    // arg_proxy_default --------------------------------------------
    template <typename T, typename... Args>
    struct arg_proxy_value {
      std::tuple<Args...> args;
    private:
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

    template <typename T, typename Alloc, typename... Args>
    struct arg_proxy_value<std::vector<T,Alloc>,Args...> {
      std::tuple<Args...> args;
    private:
      // http://stackoverflow.com/a/7858971/2640636
      template <int ...> struct seq { };
      template <int N, int ...S> struct gens : gens<N-1, N-1, S...> { };
      template <int ...S> struct gens<0, S...> { typedef seq<S...> type; };
      template <int N> using gens_t = typename gens<N>::type;

      template <int ...S>
      inline void assign_default_impl(std::vector<T,Alloc>& x, seq<S...>) const {
        x = {std::get<S>(args)...};
      }
    public:
      inline void assign_default(void* ptr) const {
        assign_default_impl( *reinterpret_cast<std::vector<T,Alloc>*>(ptr),
                             gens_t<sizeof...(Args)>() );
      }
    };

    // arg_proxy_parser ---------------------------------------------
    template <typename T>
    struct arg_proxy_parser_default {
      inline void parse(void* ptr, const std::string& str) const {
        // (*reinterpret_cast<T*>(ptr)) = boost::lexical_cast<T>(str);
        std::stringstream(str) >> (*reinterpret_cast<T*>(ptr));
      }
    };
    template <typename T, typename Alloc>
    struct arg_proxy_parser_default<std::vector<T,Alloc>> {
      inline void parse(void* ptr, const std::string& str) const {
        T x;
        std::stringstream(str) >> x;
        reinterpret_cast<std::vector<T,Alloc>*>(ptr)->emplace_back(x);
      }
    };

    template <typename T, typename Parser>
    struct arg_proxy_parser {
      Parser parser;
      inline void parse(void* ptr, const std::string& str) const {
        parser(reinterpret_cast<T*>(ptr),str);
      }
    };

    // --------------------------------------------------------------
    template <typename T>
    struct arg_proxy: arg_proxy_base, arg_proxy_parser_default<T> {
      using arg_proxy_base::arg_proxy_base;
      virtual void parse(void* ptr, const std::string& str) const {
        arg_proxy_parser_default<T>::parse(ptr,str);
      }
    };

    template <typename T, typename... Args>
    struct arg_proxy_v: arg_proxy<T>, arg_proxy_value<T,Args...> {
      template <typename S1, typename S2, typename Tuple>
      arg_proxy_v(S1&& opt, S2&& desc, flags_t flags, const Tuple& args)
      : arg_proxy<T>(std::forward<S1>(opt), std::forward<S2>(desc), flags),
        arg_proxy_value<T,Args...>{args} { }
      virtual void assign_default(void* ptr) const {
        arg_proxy_value<T,Args...>::assign_default(ptr);
      }
    };

    template <typename T, typename Parser>
    struct arg_proxy_p: arg_proxy_base, arg_proxy_parser<T,Parser> {
      template <typename S1, typename S2, typename Func>
      arg_proxy_p(S1&& opt, S2&& desc, flags_t flags, Func&& parser)
      : arg_proxy_base(std::forward<S1>(opt), std::forward<S2>(desc), flags),
        arg_proxy_parser<T,Parser>{std::forward<Func>(parser)} { }
      virtual void parse(void* ptr, const std::string& str) const {
        arg_proxy_parser<T,Parser>::parse(ptr,str);
      }
    };

    template <typename T, typename Parser, typename... Args>
    struct arg_proxy_vp: arg_proxy_base, arg_proxy_value<T,Args...>,
      arg_proxy_parser<T,Parser>
    {
      template <typename S1, typename S2, typename Tuple, typename Func>
      arg_proxy_vp(S1&& opt, S2&& desc, flags_t flags,
        const Tuple& tuple, Func&& parser)
      : arg_proxy_base(std::forward<S1>(opt), std::forward<S2>(desc), flags),
        arg_proxy_value<T,Args...>{tuple},
        arg_proxy_parser<T,Parser>{std::forward<Func>(parser)} { }
      virtual void assign_default(void* ptr) const {
        arg_proxy_value<T,Args...>::assign_default(ptr);
      }
      virtual void parse(void* ptr, const std::string& str) const {
        arg_proxy_parser<T,Parser>::parse(ptr,str);
      }
    };

    std::unordered_map<void*,std::unique_ptr<arg_proxy_base>> argmap;
    // value needs to be a pointer for polymorphism to work
    // TODO: make sure this is the best way

    void argmap_add(void* ptr, arg_proxy_base* proxy) {
      argmap.emplace( std::piecewise_construct,
        std::forward_as_tuple(ptr),
        std::forward_as_tuple(proxy)
      );
    }

    template <typename S1, typename S2>
    using call_enable = typename std::enable_if<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value,
    args_parse >::type;

    template <typename S1, typename S2, typename Func, typename T>
    using call_enable_p = typename std::enable_if<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value &&
      is_callable<Func,T*,const std::string&>::value,
    args_parse >::type;

    DEFINE_IS_TEMPLATE_TRAIT(std::tuple,std_tuple);

    template <typename S1, typename S2, typename Tuple>
    using call_enable_v1 = typename std::enable_if<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value &&
      !is_std_tuple<Tuple>::value,
    args_parse >::type;

    template <typename S1, typename S2, typename Tuple, typename Func, typename T>
    using call_enable_v1p = typename std::enable_if<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value &&
      !is_std_tuple<Tuple>::value &&
      is_callable<Func,T*,const std::string&>::value,
    args_parse >::type;

  public:
    args_parse() = default;
    args_parse& parse(int argc, char const **argv);
    // TODO: implement parsing function in .cc

    template <typename T, typename S1, typename S2>
    call_enable<S1,S2>&
    operator()( S1&& option, T* x, S2&& desc, flags_t flags=flags_t::none) {
      argmap_add(x, new arg_proxy<T>(
        std::forward<S1>(option), std::forward<S2>(desc), flags) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename... Args>
    call_enable<S1,S2>&
    operator()( S1&& option, T* x, S2&& desc, const std::tuple<Args...>& args,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_v<T, remove_cvr_t<Args>...>(
        std::forward<S1>(option), std::forward<S2>(desc), flags, args) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename Arg>
    call_enable_v1<S1,S2,Arg>&
    operator()( S1&& option, T* x, S2&& desc, const Arg& arg,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_v<T,Arg>(
        std::forward<S1>(option), std::forward<S2>(desc), flags,
        std::forward_as_tuple(arg) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename Parser>
    call_enable_p<S1,S2,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_p<T, remove_cvr_t<Parser>>(
        std::forward<S1>(option), std::forward<S2>(desc), flags, parser ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename... Args>
    call_enable_p<S1,S2,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc,
      const std::tuple<Args...>& args, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x,
        new arg_proxy_vp<T, remove_cvr_t<Parser>, remove_cvr_t<Args>...>(
          std::forward<S1>(option), std::forward<S2>(desc), flags,
          args, parser ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename Arg>
    call_enable_v1p<S1,S2,Arg,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc, const Arg& arg, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x,
        new arg_proxy_vp<T, remove_cvr_t<Parser>, Arg>(
          std::forward<S1>(option), std::forward<S2>(desc), flags,
          std::forward_as_tuple(arg), parser ) );
      return *this;
    }

  }; // end args_parse class

}} // end namespace

#endif
