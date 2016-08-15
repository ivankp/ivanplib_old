// Written by Ivan Pogrebnyak

#ifndef IVANP_ARGS_PARSE_HH
#define IVANP_ARGS_PARSE_HH

#include <utility>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>

#include <sstream>

// #include <boost/lexical_cast.hpp>

#include "extra_traits.hh"
#include "expression_traits.hh"

// TODO: switch to string_view

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

    // arg_proxy ----------------------------------------------------
    struct arg_proxy_base {
      std::string opt, desc;
      flags_t flags;
      int count;

      template <typename S1, typename S2>
      arg_proxy_base(S1&& opt, S2&& desc, flags_t flags)
      : opt(std::forward<S1>(opt)), desc(std::forward<S2>(desc)),
        flags(flags), count(0) { }

      virtual void parse(void* ptr, const std::string& str) const =0;
      virtual void assign_default(void* ptr) const { }
      virtual ~arg_proxy_base() { }
    };

    // arg_proxy_value ----------------------------------------------
    template <typename T, typename... Args>
    struct arg_proxy_value {
      mutable std::tuple<Args...> args;
      // show_type<Args...> pack_types;
      // show_type<decltype(args)> tuple_type;
      // show_type<seq<sizeof(args)>> tuple_size;
      template <size_t I>
      using type = typename std::tuple_element<I, std::tuple<Args...>>::type;

    private:
      template <typename A, size_t I> struct has_get {
        typedef char yes;
        typedef char no[2];
        template <typename U> static auto f(U&& x)
          -> decltype( std::get<I>(x), yes() );
        template <typename U> static no&  f(...);
        enum { value = sizeof(f<A>(std::declval<A>())) == sizeof(yes) };
      };

      template <size_t I>
      inline typename std::enable_if<(I==sizeof...(Args)-1)>::type
      assign_default_impl_get(T& x) const {
        std::get<I>(x) = std::forward<type<I>>(std::get<I>(args));
      }
      template <size_t I>
      inline typename std::enable_if<(I!=sizeof...(Args)-1)>::type
      assign_default_impl_get(T& x) const {
        std::get<I>(x) = std::forward<type<I>>(std::get<I>(args));
        assign_default_impl_get<I+1>(x);
      }

      template <size_t... I>
      inline typename std::enable_if<(sizeof...(I)!=1)>::type
      assign_default_impl(T& x, seq<I...>) const {
        x = T(std::forward<type<I>...>(std::get<I>(args))...);
      }
      template <size_t... I>
      inline typename std::enable_if<(sizeof...(I)==1)>::type
      assign_default_impl(T& x, seq<I...>) const {
        x = std::forward<type<I>...>(std::get<0>(args));
      }
    public:
      template <typename U=T>
      inline typename std::enable_if< !has_get<U,0>::value >::type
      assign_default(void* ptr) const {
        assign_default_impl( *reinterpret_cast<U*>(ptr),
                             seq_up_to<sizeof...(Args)>() );
      }
      template <typename U=T>
      inline typename std::enable_if<  has_get<U,0>::value >::type
      assign_default(void* ptr) const {
        assign_default_impl_get<0>( *reinterpret_cast<U*>(ptr) );
      }
    };

    template <typename T, typename Alloc, typename... Args>
    struct arg_proxy_value<std::vector<T,Alloc>,Args...> {
      mutable std::tuple<Args...> args;
      template <size_t I>
      using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    private:
      template <size_t... I>
      inline void assign_default_impl(std::vector<T,Alloc>& x, seq<I...>) const {
        x = { std::forward<type<I>>(std::get<I>(args))... };
      }
    public:
      template <typename U=T>
      inline typename std::enable_if<
        sizeof...(Args)!=1 || !std::is_same< type<0>, U >::value
      >::type assign_default(void* ptr) const {
        assign_default_impl( *reinterpret_cast<std::vector<U,Alloc>*>(ptr),
                             seq_up_to<sizeof...(Args)>() );
      }
      template <typename U=T>
      inline typename std::enable_if<
        sizeof...(Args)==1 && std::is_same< type<0>, U >::value
      >::type assign_default(void* ptr) const {
        *reinterpret_cast<std::vector<U,Alloc>*>(ptr)
          = std::forward<type<0>>(std::get<0>(args));
      }
    };

    // TODO: default from functional

    // arg_proxy_parser ---------------------------------------------
    // TODO: refrain from stringstreams, parse without copying strings
    template <typename T>
    inline static void default_parse(T& x, const std::string& str) {
      // (*reinterpret_cast<T*>(ptr)) = boost::lexical_cast<T>(str);
      std::stringstream(str) >> x;
    }
    template <typename T, typename InputIt>
    inline static void default_parse(T& x, InputIt first, InputIt last) {
      std::stringstream ss;
      std::copy(first, last, std::ostreambuf_iterator<char>(ss));
      ss >> x;
    }

    template <typename T, typename = void>
    struct arg_proxy_parser_default {
      inline static void parse(void* ptr, const std::string& str) {
        default_parse(*reinterpret_cast<T*>(ptr),str);
      }
    };

    template <typename T, size_t N>
    struct arg_proxy_parser_default<std::array<T,N>> {
      using iter_t = std::string::const_iterator;

      template <size_t I>
      inline static typename std::enable_if<(I<N-1)>::type
      parse_impl(std::array<T,N>& x, iter_t begin, iter_t end) {
        auto delim = std::find(begin,end,':');
        default_parse(std::get<I>(x),begin,delim);
        if (delim!=end) parse_impl<I+1>(x,++delim,end);
      }
      template <size_t I>
      inline static typename std::enable_if<(I==N-1)>::type
      parse_impl(std::array<T,N>& x, iter_t begin, iter_t end) {
        default_parse(std::get<I>(x),begin,end);
      }
      inline static void parse(void* ptr, const std::string& str) {
        parse_impl<0>(*reinterpret_cast<std::array<T,N>*>(ptr),
                      str.begin(), str.end() );
      }
    };

    template <typename... TT>
    struct arg_proxy_parser_default<std::tuple<TT...>> {
      using iter_t = std::string::const_iterator;

      template <size_t I>
      inline static typename std::enable_if<(I<sizeof...(TT)-1)>::type
      parse_impl(std::tuple<TT...>& x, iter_t begin, iter_t end) {
        auto delim = std::find(begin,end,':');
        default_parse(std::get<I>(x),begin,delim);
        if (delim!=end) parse_impl<I+1>(x,++delim,end);
      }
      template <size_t I>
      inline static typename std::enable_if<(I==sizeof...(TT)-1)>::type
      parse_impl(std::tuple<TT...>& x, iter_t begin, iter_t end) {
        default_parse(std::get<I>(x),begin,end);
      }
      inline static void parse(void* ptr, const std::string& str) {
        parse_impl<0>(*reinterpret_cast<std::tuple<TT...>*>(ptr),
                      str.begin(), str.end() );
      }
    };

    template <typename... TT>
    struct arg_proxy_parser_default<std::pair<TT...>> {
      using iter_t = std::string::const_iterator;

      template <size_t I>
      inline static typename std::enable_if<(I==0)>::type
      parse_impl(std::pair<TT...>& x, iter_t begin, iter_t end) {
        auto delim = std::find(begin,end,':');
        default_parse(std::get<I>(x),begin,delim);
        if (delim!=end) parse_impl<I+1>(x,++delim,end);
      }
      template <size_t I>
      inline static typename std::enable_if<(I==1)>::type
      parse_impl(std::pair<TT...>& x, iter_t begin, iter_t end) {
        default_parse(std::get<I>(x),begin,end);
      }
      inline static void parse(void* ptr, const std::string& str) {
        parse_impl<0>(*reinterpret_cast<std::pair<TT...>*>(ptr),
                      str.begin(), str.end() );
      }
    };

    DEFINE_BINARY_TRAIT(has_emplace_back, x1.emplace_back(x2));
    DEFINE_BINARY_TRAIT(has_emplace, x1.emplace(x2));
    DEFINE_BINARY_TRAIT(has_emplace_front, x1.emplace_front(x2));

    template <typename T, typename = void>
    struct emplace_trait { enum { value = false }; };

    template <typename T>
    struct emplace_trait<T, typename std::enable_if<
      has_emplace_back<T, typename T::value_type>::value
    >::type> {
      enum { value = true };
      using type = typename T::value_type;
      template <typename... Args>
      inline static void emplace(T* x, Args&&... args) {
        x->emplace_back(std::forward<Args>(args)...);
      }
    };

    template <typename T>
    struct emplace_trait<T, typename std::enable_if<
      !has_emplace_back<T, typename T::value_type>::value &&
      has_emplace<T, typename T::value_type>::value
    >::type> {
      enum { value = true };
      using type = typename T::value_type;
      template <typename... Args>
      inline static void emplace(T* x, Args&&... args) {
        x->emplace(std::forward<Args>(args)...);
      }
    };

    template <typename T>
    struct emplace_trait<T, typename std::enable_if<
      !has_emplace_back<T, typename T::value_type>::value &&
      !has_emplace<T, typename T::value_type>::value &&
      has_emplace_front<T, typename T::value_type>::value
    >::type> {
      enum { value = true };
      using type = typename T::value_type;
      template <typename... Args>
      inline static void emplace(T* x, Args&&... args) {
        x->emplace_front(std::forward<Args>(args)...);
      }
    };

    template <typename T>
    struct arg_proxy_parser_default<T, typename std::enable_if<
      emplace_trait<T>::value
    >::type> {
      inline void parse(void* ptr, const std::string& str) const {
        typename emplace_trait<T>::type x;
        arg_proxy_parser_default<decltype(x)>::parse(&x,str);
        emplace_trait<T>::emplace(reinterpret_cast<T*>(ptr),std::move(x));
      }
    };

    template <typename T, typename Parser>
    struct arg_proxy_parser {
      Parser parser;
      inline void parse(void* ptr, const std::string& str) const {
        parser(reinterpret_cast<T*>(ptr),str);
      }
    };
    // TODO: specialize for by-element parsers

    // template <typename T, typename Parser>
    // struct arg_proxy_element_parser {
    //   Parser parser;
    //
    //   DEFINE_BINARY_TRAIT(has_emplace_back, x1.emplace_back(x2));
    //   DEFINE_BINARY_TRAIT(has_emplace, x1.emplace(x2));
    //
    //   template <typename U>
    //   using enable_if_emplace_back_t = typename std::enable_if<
    //     has_emplace_back<U,
    //       decltype(parser(std::declval<const std::string&>()))
    //     >::value
    //   >::type;
    //   template <typename U>
    //   using enable_if_emplace_t = typename std::enable_if<
    //     has_emplace<U,
    //       decltype(parser(std::declval<const std::string&>()))
    //     >::value
    //   >::type;
    //
    //   template <typename U> inline enable_if_emplace_back_t<U>
    //   parse_impl(void* ptr, const std::string& str) const {
    //     reinterpret_cast<U*>(ptr)->emplace_back(parser(str));
    //   }
    //   template <typename U> inline enable_if_emplace_t<U>
    //   parse_impl(void* ptr, const std::string& str) const {
    //     reinterpret_cast<U*>(ptr)->emplace(parser(str));
    //   }
    //
    //   inline void parse(void* ptr, const std::string& str) const {
    //     parse_impl<T>(str);
    //   }
    // };
    //
    // template <typename T, typename Parser, typename Alloc>
    // struct arg_proxy_parser<std::vector<T,Alloc>> {
    //   Parser parser;
    //   inline void parse(void* ptr, const std::string& str) const {
    //     reinterpret_cast<std::vector<T,Alloc>*>(ptr)->emplace_back(parser(str));
    //   }
    // };

    // --------------------------------------------------------------
    template <typename T, typename = void>
    struct type_flags_trait { static constexpr auto value = flags_t::none; };

    template <typename T>
    struct type_flags_trait<T, typename std::enable_if<
      emplace_trait<T>::value
    >::type> { static constexpr auto value = flags_t::multiple; };

    template <typename T>
    inline flags_t type_flags(flags_t flags) const noexcept {
      return static_cast<flags_t>(flags | type_flags_trait<T>::value);
    }

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
      arg_proxy_v(S1&& opt, S2&& desc, flags_t flags, Tuple&& args)
      : arg_proxy<T>(std::forward<S1>(opt), std::forward<S2>(desc), flags),
        arg_proxy_value<T,Args...>{std::forward<Tuple>(args)} { }
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
        Tuple&& args, Func&& parser)
      : arg_proxy_base(std::forward<S1>(opt), std::forward<S2>(desc), flags),
        arg_proxy_value<T,Args...>{std::forward<Tuple>(args)},
        arg_proxy_parser<T,Parser>{std::forward<Func>(parser)} { }
      virtual void assign_default(void* ptr) const {
        arg_proxy_value<T,Args...>::assign_default(ptr);
      }
      virtual void parse(void* ptr, const std::string& str) const {
        arg_proxy_parser<T,Parser>::parse(ptr,str);
      }
    };
    // --------------------------------------------------------------

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
    using call_enable_t = typename std::enable_if<
      std::is_convertible<S1,std::string>::value &&
      std::is_convertible<S2,std::string>::value,
    args_parse >::type;

    template <typename S1, typename S2, typename Func, typename T>
    using call_enable_p_t = typename std::enable_if<
      is_callable<Func,T*,const std::string&>::value,
    call_enable_t<S1,S2> >::type;

    template <typename S1, typename S2, typename Arg, typename T>
    using call_enable_v1_t = typename std::enable_if<
      std::is_convertible<Arg,T>::value,
    call_enable_t<S1,S2> >::type;

    template <typename S1, typename S2, typename Arg, typename Func, typename T>
    using call_enable_v1p_t = typename std::tuple_element<0,std::pair<
      call_enable_p_t<S1,S2,Func,T>,
      call_enable_v1_t<S1,S2,Arg,T>
    >>::type;

  public:
    args_parse() = default;
    args_parse& parse(int argc, char const **argv);

    template <typename T, typename S1, typename S2>
    call_enable_t<S1,S2>&
    operator()( S1&& option, T* x, S2&& desc, flags_t flags=flags_t::none) {
      argmap_add(x, new arg_proxy<T>(
        std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags)) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename... Args>
    call_enable_t<S1,S2>&
    operator()( S1&& option, T* x, S2&& desc, const std::tuple<Args...>& args,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_v<T, remove_rvalue_reference_t<Args>...>(
        std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
        args) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename... Args>
    call_enable_t<S1,S2>&
    operator()( S1&& option, T* x, S2&& desc, std::tuple<Args...>&& args,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_v<T, remove_rvalue_reference_t<Args>...>(
        std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
        std::move(args)) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename Arg>
    call_enable_v1_t<S1,S2,Arg,T>&
    operator()( S1&& option, T* x, S2&& desc, Arg&& arg,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_v<T,Arg>(
        std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
        std::forward_as_tuple(std::forward<Arg>(arg)) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2, typename Parser>
    call_enable_p_t<S1,S2,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc, no_default_t, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x, new arg_proxy_p<T, Parser>(
        std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
        std::forward<Parser>(parser) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename... Args>
    call_enable_p_t<S1,S2,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc,
      const std::tuple<Args...>& args, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x,
        new arg_proxy_vp<T, Parser, remove_rvalue_reference_t<Args>...>(
          std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
          args, std::forward<Parser>(parser) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename... Args>
    call_enable_p_t<S1,S2,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc,
      std::tuple<Args...>&& args, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x,
        new arg_proxy_vp<T, Parser, remove_rvalue_reference_t<Args>...>(
          std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
          std::move(args), std::forward<Parser>(parser) ) );
      return *this;
    }

    template <typename T, typename S1, typename S2,
              typename Parser, typename Arg>
    call_enable_v1p_t<S1,S2,Arg,Parser,T>&
    operator()( S1&& option, T* x, S2&& desc, Arg&& arg, Parser&& parser,
      flags_t flags=flags_t::none)
    {
      argmap_add(x,
        new arg_proxy_vp<T, Parser, Arg>(
          std::forward<S1>(option), std::forward<S2>(desc), type_flags<T>(flags),
          std::forward_as_tuple(std::forward<Arg>(arg)),
          std::forward<Parser>(parser) ) );
      return *this;
    }

  }; // end args_parse class

}} // end namespace

#endif
