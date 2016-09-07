// Written by Ivan Pogrebnyak

#ifndef IVANP_ARGS_PARSE_HH
#define IVANP_ARGS_PARSE_HH

#include <utility>
#include <memory>
#include <algorithm>
#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <stdexcept>

#ifdef ARGS_PARSE_USE_BOOST_LEXICAL_CAST
#include <boost/lexical_cast.hpp>
#else
#include <sstream>
#endif

#include "extra_traits.hh"
#include "expression_traits.hh"
#include "emplace_traits.hh"

// TODO: check for temporary copies with containers
// TODO: switch to char*
// TODO: positional arguments
// TODO: write the algorithm

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
    using siter_t = std::string::const_iterator;

    // arg_proxy_base -----------------------------------------------
    struct arg_proxy_base {
      void* ptr;
      flags_t flags;
      int count;

      arg_proxy_base(void* ptr, flags_t flags)
      : ptr(ptr), flags(flags), count(0) { }

      virtual void parse(const std::string& str) const =0;
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
      inline static void parse(T& x, siter_t first, siter_t last) {
        #ifdef ARGS_PARSE_USE_BOOST_LEXICAL_CAST
          x = boost::lexical_cast<T>(&*first,last-first);
        #else
          std::stringstream ss;
          std::copy(first, last, std::ostreambuf_iterator<char>(ss));
          ss >> x;
        #endif
      }
      inline static void parse(void* ptr, const std::string& str) {
        parse(*reinterpret_cast<T*>(ptr),str.begin(),str.end());
      }
    };

    // tuples, arrays, pairs
    template <typename T>
    struct arg_parser_default<T, enable_if_t<std::tuple_size<T>::value>> {
      enum { size = std::tuple_size<T>::value };
      template <size_t I> using type = tuple_element_t<I,T>;

      template <size_t I> inline static enable_if_t<(I<size-1)>
      parse_impl(T& x, siter_t begin, siter_t end) {
        auto delim = std::find(begin,end,':');
        arg_parser_default<type<I>>::parse(std::get<I>(x),begin,delim);
        if (delim!=end) parse_impl<I+1>(x,++delim,end);
      }
      template <size_t I> inline static enable_if_t<(I==size-1)>
      parse_impl(T& x, siter_t begin, siter_t end) {
        arg_parser_default<type<I>>::parse(std::get<I>(x),begin,end);
      }
      inline static void parse(T& x, siter_t first, siter_t last) {
        parse_impl<0>(x,first,last);
      }
      inline static void parse(void* ptr, const std::string& str) {
        parse_impl<0>(*reinterpret_cast<T*>(ptr), str.begin(), str.end() );
      }
    };

    // containters
    template <typename T>
    struct arg_parser_default<T, enable_if_t<can_emplace<T>::value>> {
      inline static void parse(T& xx, siter_t begin, siter_t end) {
        typename can_emplace<T>::type x;
        arg_parser_default<decltype(x)>::parse(x,begin,end);
        can_emplace<T>::emplace(&xx,std::move(x));
      }
      inline static void parse(void* ptr, const std::string& str) {
        parse(*reinterpret_cast<T*>(ptr),str.begin(),str.end());
      }
    };

    template <typename T, typename Parser>
    struct arg_parser {
      Parser parser;
      inline void parse(void* ptr, const std::string& str) const {
        parser(reinterpret_cast<T*>(ptr),str);
      }
    };

    // arg_flags ----------------------------------------------------
    template <typename T, typename = void>
    struct arg_flags_impl { static constexpr auto value = flags_t::none; };

    template <typename T>
    struct arg_flags_impl<T, enable_if_t<can_emplace<T>::value>> {
      static constexpr auto value = flags_t::multiple;
    };

    template <typename T>
    inline flags_t arg_flags(flags_t flags) const noexcept {
      return static_cast<flags_t>(flags | arg_flags_impl<T>::value);
    }

    // arg_proxy ----------------------------------------------------
    template <typename T>
    struct arg_proxy: arg_proxy_base, arg_parser_default<T> {
      using arg_proxy_base::arg_proxy_base;
      virtual void parse(const std::string& str) const {
        arg_parser_default<T>::parse(arg_proxy_base::ptr, str);
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
      virtual void parse(const std::string& str) const {
        arg_parser<T,Parser>::parse(arg_proxy_base::ptr,str);
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
      virtual void parse(const std::string& str) const {
        arg_parser<T,Parser>::parse(arg_proxy_base::ptr,str);
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
      is_callable<Func,T*,const std::string&>::value,
    call_enable_t<S1,S2> >;

    template <typename S1, typename S2, typename Arg, typename T>
    using call_enable_v1_t = enable_if_t<
      std::is_convertible<Arg,T>::value,
    call_enable_t<S1,S2> >;

    template <typename S1, typename S2, typename Arg, typename Func, typename T>
    using call_enable_v1p_t = typename std::tuple_element<0,std::pair<
      call_enable_p_t<S1,S2,Func,T>,
      call_enable_v1_t<S1,S2,Arg,T>
    >>::type;

    // argmap -------------------------------------------------------
    // std::unordered_map<void*,std::unique_ptr<arg_proxy_base>> argmap;
    // value needs to be a pointer for polymorphism to work
    // TODO: make sure this is the best way
    // TODO: find out why argmap needs to be an unordered_map

    // void add_arg(void* ptr, arg_proxy_base* proxy) {
    //   argmap.emplace( std::piecewise_construct,
    //     std::forward_as_tuple(ptr),
    //     std::forward_as_tuple(proxy)
    //   );
    // }

    std::vector<arg_proxy_base*> arg_proxies;
    std::map<std::string,arg_proxy_base*> long_argmap;
    std::map<char,arg_proxy_base*> short_argmap;
    std::vector<std::pair<std::string,std::string>> arg_descs;

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
    }

  public:
    args_parse() = default;
    args_parse& parse(int argc, char const * const * argv);
    ~args_parse() { for (auto* proxy : arg_proxies) delete proxy; }

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
