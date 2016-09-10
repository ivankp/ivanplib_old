// Written by Ivan Pogrebnyak

#ifndef IVANP_ARGS_PARSE_HH
#define IVANP_ARGS_PARSE_HH

#include <utility>
#include <memory>
#include <algorithm>
#include <string>
#include <tuple>
#include <list>
#include <map>
#include <stdexcept>
#include <iostream>

#ifdef ARGS_PARSE_USE_BOOST_LEXICAL_CAST
  #include <boost/lexical_cast.hpp>
#else
  #include <streambuf>
#endif

#include "extra_traits.hh"
#include "expression_traits.hh"
#include "emplace_traits.hh"

// TODO: write the algorithm
// TODO: positional arguments
// TODO: multivalue

// TODO: help
// TODO: check for temporary copies with containers
// TODO: avoid copying option & description strings

// TODO: default functors
// TODO: implement by-element parser passing

namespace ivanp { namespace args_parse {

  enum flags_t : unsigned short {
    none = 0,
    required = 1,
    multiple = 2,
    multivalue = 4 // TODO: implement
  };

  struct no_default_t { };
  constexpr no_default_t no_default;

  class args_parse {
    struct arg_proxy_base;

    std::vector<arg_proxy_base*> arg_proxies;
    std::map<std::string,arg_proxy_base*> long_argmap;
    std::map<char,arg_proxy_base*> short_argmap;
    std::list<std::pair<std::string,std::string>> arg_descs;

    // arg_proxy_base -----------------------------------------------
    struct arg_proxy_base {
      void* ptr;
      flags_t flags;
      int count;
      decltype(arg_descs)::const_pointer opt;

      arg_proxy_base(void* ptr, flags_t flags)
      : ptr(ptr), flags(flags), count(0) { }

      virtual void parse(const char* str, size_t n) const =0;
      virtual void assign_default() const { }
      virtual ~arg_proxy_base() { }
    };

    // arg_value ----------------------------------------------
    template <typename T, typename... Args>
    struct arg_value {
      mutable std::tuple<Args...> args;
    private:
      template <size_t I>
      using type = typename std::tuple_element<I, std::tuple<Args...>>::type;

      template <typename A, size_t I> struct has_get {
        typedef char yes;
        typedef char no[2];
        template <typename U> static auto f(U&& x)
          -> decltype( std::get<I>(x), yes() );
        template <typename U> static no&  f(...);
        enum { value = sizeof(f<A>(std::declval<A>())) == sizeof(yes) };
      };

      template <size_t... I> inline enable_if_t<
        sizeof...(I)==1 && !can_emplace<T>::value && !has_get<T,0>::value
      > assign_default_impl(T& x, seq<I...>) const {
        x = std::forward<type<0>>(std::get<0>(args));
      }
      template <size_t... I> inline enable_if_t<
        (sizeof...(I)!=1 || can_emplace<T>::value) && !has_get<T,0>::value
      > assign_default_impl(T& x, seq<I...>) const {
        x = {std::forward<type<I>>(std::get<I>(args))...};
      }
      template <size_t... I> inline enable_if_t<
        sizeof...(I) && has_get<T,0>::value
      > assign_default_impl(T& x, seq<I...>) const {
        std::tie(std::get<I>(x)...) = std::forward_as_tuple(
          std::forward<type<I>>(std::get<I>(args))... );
      }
    public:
      inline void assign_default(void* ptr) const {
        assign_default_impl( *reinterpret_cast<T*>(ptr),
                             seq_up_to<sizeof...(Args)>() );
      }
    };

    // arg_parser ---------------------------------------------
    template <typename T, typename = void>
    struct arg_parser_default {
      struct membuf : std::streambuf {
        membuf(const char* str, size_t n) {
          char *p = const_cast<char*>(str);
          this->setg(p, p, p+n);
        }
      };

      inline static void parse(T& x, const char* str, size_t n) {
        #ifdef ARGS_PARSE_USE_BOOST_LEXICAL_CAST
          x = boost::lexical_cast<T>(str,n);
        #else
          membuf buf(str,n);
          std::istream(&buf) >> x;
        #endif
      }
      [[ gnu::flatten ]]
      inline static void parse(void* ptr, const char* str, size_t n) {
        parse(*reinterpret_cast<T*>(ptr),str,n);
      }
    };

    // tuples, arrays, pairs
    template <typename T>
    struct arg_parser_default<T, enable_if_t<std::tuple_size<T>::value>> {
      enum { size = std::tuple_size<T>::value };
      template <size_t I> using type = tuple_element_t<I,T>;

      template <size_t I>
      [[ gnu::flatten ]] inline static enable_if_t<(I<size-1)>
      parse_impl(T& x, const char* str, size_t n) {
        const char* delim = std::find(str,str+n,':');
        const size_t n1 = delim-str;
        const size_t n2 = n-n1-1;
        arg_parser_default<type<I>>::parse(std::get<I>(x),str,n1);
        if (n1!=n) parse_impl<I+1>(x,delim+1,n2);
      }
      template <size_t I>
      [[ gnu::flatten ]] inline static enable_if_t<(I==size-1)>
      parse_impl(T& x, const char* str, size_t n) {
        arg_parser_default<type<I>>::parse(std::get<I>(x),str,n);
      }
      [[ gnu::flatten ]]
      inline static void parse(T& x, const char* str, size_t n) {
        parse_impl<0>(x,str,n);
      }
      [[ gnu::flatten ]]
      inline static void parse(void* ptr, const char* str, size_t n) {
        parse_impl<0>(*reinterpret_cast<T*>(ptr),str,n);
      }
    };

    // containters
    template <typename T>
    struct arg_parser_default<T, enable_if_t<can_emplace<T>::value>> {
      [[ gnu::flatten ]]
      inline static void parse(T& xx, const char* str, size_t n) {
        typename can_emplace<T>::type x;
        arg_parser_default<decltype(x)>::parse(x,str,n);
        can_emplace<T>::emplace(&xx,std::move(x));
      }
      [[ gnu::flatten ]]
      inline static void parse(void* ptr, const char* str, size_t n) {
        parse(*reinterpret_cast<T*>(ptr),str,n);
      }
    };

    // user parser
    template <typename P, typename T> struct is_parser_direct {
      typedef char yes;
      typedef char no[2];
      template <typename P1, typename T1> static auto f(P1&& p, T* x)
      -> decltype( p(x,std::declval<const char*>(),size_t()), yes() );
      template <typename P1, typename T1> static no& f(...);
      enum { value = sizeof(f<P,T>(std::declval<P>(),std::declval<T*>()))
                  == sizeof(yes) };
    };
    template <typename P, typename T> struct is_parser_view {
      typedef char yes;
      typedef char no[2];
      template <typename P1, typename T1> static auto f(P1&& p, T* x)
      -> decltype( p(x,{std::declval<const char*>(),size_t()}), yes() );
      template <typename P1, typename T1> static no& f(...);
      enum { value = sizeof(f<P,T>(std::declval<P>(),std::declval<T*>()))
                  == sizeof(yes) };
    };
    template <typename P, typename T> struct is_parser {
      enum { value = is_parser_direct<P,T>::value
                  || is_parser_view<P,T>::value };
    };

    template <typename T, typename Parser, typename = void>
    struct arg_parser { // for string_view
      Parser parser;
      [[ gnu::flatten ]]
      inline void parse(void* ptr, const char* str, size_t n) const {
        parser(reinterpret_cast<T*>(ptr),{str,n});
      }
    };

    template <typename T, typename Parser>
    struct arg_parser<T,Parser,enable_if_t<is_parser_direct<Parser,T>::value>>
    { // for direct call with pointer and size
      Parser parser;
      [[ gnu::flatten ]]
      inline void parse(void* ptr, const char* str, size_t n) const {
        parser(reinterpret_cast<T*>(ptr),str,n);
      }
    };

    // arg_flags ----------------------------------------------------
    template <typename T, typename = void>
    struct arg_flags_impl { static constexpr auto value = flags_t::none; };

    template <typename T>
    struct arg_flags_impl<T, enable_if_t<can_emplace<T>::value>> {
      static constexpr auto value = flags_t::multiple;
    };

    template <typename T> [[ gnu::pure ]]
    static inline flags_t arg_flags(flags_t flags) noexcept {
      return static_cast<flags_t>(flags | arg_flags_impl<T>::value);
    }

    // arg_proxy ----------------------------------------------------
    template <typename T>
    struct arg_proxy: arg_proxy_base, arg_parser_default<T> {
      using arg_proxy_base::arg_proxy_base;
      virtual void parse(const char* str, size_t n) const {
        arg_parser_default<T>::parse(arg_proxy_base::ptr,str,n);
      }
    };

    template <typename T, typename... Args>
    struct arg_proxy_v: arg_proxy<T>, arg_value<T,Args...> {
      template <typename Tuple>
      arg_proxy_v(void* ptr, flags_t flags, Tuple&& args)
      : arg_proxy<T>(ptr, flags),
        arg_value<T,Args...>{std::forward<Tuple>(args)} { }
      virtual void assign_default() const {
        arg_value<T,Args...>::assign_default(arg_proxy_base::ptr);
      }
    };

    template <typename T, typename Parser>
    struct arg_proxy_p: arg_proxy_base, arg_parser<T,Parser> {
      template <typename Func>
      arg_proxy_p(void* ptr, flags_t flags, Func&& parser)
      : arg_proxy_base(ptr, flags),
        arg_parser<T,Parser>{std::forward<Func>(parser)} { }
      virtual void parse(const char* str, size_t n) const {
        arg_parser<T,Parser>::parse(arg_proxy_base::ptr,str,n);
      }
    };

    template <typename T, typename Parser, typename... Args>
    struct arg_proxy_vp: arg_proxy_base, arg_value<T,Args...>,
      arg_parser<T,Parser>
    {
      template <typename Tuple, typename Func>
      arg_proxy_vp(void* ptr, flags_t flags, Tuple&& args, Func&& parser)
      : arg_proxy_base(ptr, flags),
        arg_value<T,Args...>{std::forward<Tuple>(args)},
        arg_parser<T,Parser>{std::forward<Func>(parser)} { }
      virtual void assign_default() const {
        arg_value<T,Args...>::assign_default(arg_proxy_base::ptr);
      }
      virtual void parse(const char* str, size_t n) const {
        arg_parser<T,Parser>::parse(arg_proxy_base::ptr,str,n);
      }
    };

    // call_enable --------------------------------------------------
    template <typename S1, typename S2>
    using call_enable_t = enable_if_t<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value,
    args_parse >;

    template <typename S1, typename S2, typename Func, typename T>
    using call_enable_p_t = enable_if_t<
      is_parser<Func,T>::value, call_enable_t<S1,S2> >;

    template <typename S1, typename S2, typename Arg, typename T>
    using call_enable_v1_t = enable_if_t<
      std::is_convertible<Arg,T>::value, call_enable_t<S1,S2> >;

    template <typename S1, typename S2, typename Arg, typename Func, typename T>
    using call_enable_v1p_t = typename std::tuple_element<0,std::pair<
      call_enable_p_t<S1,S2,Func,T>,
      call_enable_v1_t<S1,S2,Arg,T>
    >>::type;

    // argmap -------------------------------------------------------
    template <typename S1, typename S2>
    void add_arg(S1&& opt, S2&& desc, arg_proxy_base* proxy) {
      arg_proxies.push_back(proxy);
      std::string opts(std::forward<S1>(opt));
      for (size_t i=0, j=opts.find(','); ; ) {
        const size_t n = j-i;

        if (n==1) {
          if (!short_argmap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(opts[i]),
            std::forward_as_tuple(proxy)).second) throw std::runtime_error(
              "duplicate option: "+opts.substr(i,n));
        } else {
          if (!long_argmap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(opts,i,n),
            std::forward_as_tuple(proxy)).second) throw std::runtime_error(
              "duplicate option: "+opts.substr(i,n));
        }

        if (j==std::string::npos) break;
        j = opts.find(',',i=j+1);
      }
      arg_descs.emplace_back(std::move(opts),std::forward<S2>(desc));
      proxy->opt = &arg_descs.back();
    }

  public:
    args_parse() = default;
    args_parse& parse(int argc, char const * const * argv);
    ~args_parse() { for (auto* proxy : arg_proxies) delete proxy; }

    bool help(int argc, char const * const * argv,
              std::string str="h,help", bool no_args_help=true,
              std::ostream& os = std::cout) const;
    void print_help(const std::string& str, std::ostream& os = std::cout) const;

    // call operators -----------------------------------------------
    template <typename T, typename S1, typename S2>
    call_enable_t<S1,S2>&
    operator()( S1&& opt, T* x, S2&& desc, flags_t flags=flags_t::none) {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy<T>(x, arg_flags<T>(flags)) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename... Args>
    call_enable_t<S1,S2>&
    operator()( S1&& opt, T* x, S2&& desc, const std::tuple<Args...>& args,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_v<T, remove_rvalue_reference_t<Args>...>(
          x, arg_flags<T>(flags), args) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename... Args>
    call_enable_t<S1,S2>&
    operator()( S1&& opt, T* x, S2&& desc, std::tuple<Args...>&& args,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_v<T, remove_rvalue_reference_t<Args>...>(
          x, arg_flags<T>(flags), std::move(args)) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename Arg>
    call_enable_v1_t<S1,S2,Arg,T>&
    operator()( S1&& opt, T* x, S2&& desc, Arg&& arg,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_v<T,Arg>(x, arg_flags<T>(flags),
          std::forward_as_tuple(std::forward<Arg>(arg)) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename Parser>
    call_enable_p_t<S1,S2,Parser,T>&
    operator()( S1&& opt, T* x, S2&& desc, no_default_t, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_p<T, Parser>(
          x, arg_flags<T>(flags), std::forward<Parser>(parser) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename... Args>
    call_enable_p_t<S1,S2,Parser,T>&
    operator()( S1&& opt, T* x, S2&& desc,
      const std::tuple<Args...>& args, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_vp<T, Parser, remove_rvalue_reference_t<Args>...>(
          x, arg_flags<T>(flags), args, std::forward<Parser>(parser) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename... Args>
    call_enable_p_t<S1,S2,Parser,T>&
    operator()( S1&& opt, T* x, S2&& desc,
      std::tuple<Args...>&& args, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_vp<T, Parser, remove_rvalue_reference_t<Args>...>(
          x, arg_flags<T>(flags), std::move(args),
          std::forward<Parser>(parser) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename Arg>
    call_enable_v1p_t<S1,S2,Arg,Parser,T>&
    operator()( S1&& opt, T* x, S2&& desc, Arg&& arg, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      add_arg(std::forward<S1>(opt), std::forward<S2>(desc),
        new arg_proxy_vp<T, Parser, Arg>(
          x, arg_flags<T>(flags), std::forward_as_tuple(std::forward<Arg>(arg)),
          std::forward<Parser>(parser) ) );
      return *this;
    }

  }; // end args_parse class

}} // end namespace

#endif
