// Written by Ivan Pogrebnyak

#ifndef IVANP_EXTRA_TRAITS_HH
#define IVANP_EXTRA_TRAITS_HH

#include <type_traits>

#define DEFINE_IS_TYPE_TRAIT(TYPE,NAME) \
template <typename T> struct is_##NAME##_impl { enum { value = 0 }; }; \
template <> struct is_##NAME##_impl<TYPE> { enum { value = 1 }; }; \
template <typename T> struct is_##NAME : is_##NAME##_impl< \
  typename std::remove_reference<typename std::remove_cv<T>::type>::type> { };

#define DEFINE_IS_TEMPLATE_TRAIT(TYPE,NAME) \
template <typename T> struct is_##NAME##_impl { enum { value = 0 }; }; \
template <typename... TT> struct is_##NAME##_impl<TYPE<TT...>> \
  { enum { value = 1 }; }; \
template <typename T> struct is_##NAME : is_##NAME##_impl< \
  typename std::remove_reference<typename std::remove_cv<T>::type>::type> { };

namespace ivanp {

#if __cplusplus < 201402L
template <bool B, typename T=void>
using enable_if_t = typename std::enable_if<B,T>::type;
#endif

// boolean compositing **********************************************

template <bool...> struct bool_sequence {};

template <bool... B>
using bool_and = std::is_same< bool_sequence< B... >,
                               bool_sequence< ( B || true )... > >;
template <bool... B>
using bool_or = std::integral_constant< bool, !bool_and< !B... >::value >;

template <template<typename> class Condition, typename... TT>
using all_are = bool_and<Condition<TT>::value...>;

// index sequencing *************************************************

// http://stackoverflow.com/a/7858971/2640636
template <size_t ...> struct seq { };
template <size_t N, size_t ...S> struct gen_seq : gen_seq<N-1, N-1, S...> { };
template <size_t ...S> struct gen_seq<0, S...> { typedef seq<S...> type; };
template <size_t N> using seq_up_to = typename gen_seq<N>::type;

// find_first_type **************************************************

template <typename T, size_t N>
struct find_first_type_impl_found {
  enum { value = N };
  using type = T;
};

template <template<typename> class Condition, typename... TT>
struct find_first_type_impl { };

template <template<typename> class Condition, typename T, typename... TT>
struct find_first_type_impl<Condition,T,TT...>: std::conditional<
  Condition<T>::value,
  find_first_type_impl_found<T,sizeof...(TT)+1>,
  find_first_type_impl<Condition,TT...>
>::type { };

template <template<typename> class Condition>
struct find_first_type_impl<Condition>: find_first_type_impl_found<void,0> { };

template <template<typename> class Condition, typename... TT>
struct find_first_type {
  enum { value = sizeof...(TT)-find_first_type_impl<Condition,TT...>::value };
  using type = typename find_first_type_impl<Condition,TT...>::type;
};

// ******************************************************************

template <typename T>
using remove_cvr_t = typename std::remove_reference<
  typename std::remove_cv<T>::type>::type;

template <typename T>
using remove_rvalue_reference_t = typename std::conditional<
  std::is_rvalue_reference<T>::value,
  typename std::remove_reference<T>::type, T
>::type;

// template <typename T>
// using nonref_to_lref_t = typename std::conditional<
//   std::is_reference<T>::value,
//   T, typename std::add_lvalue_reference<T>::type
// >::type;

// template <template<typename...> class In, typename... TT>
// struct bind_all_types_but_1 {
//   template <typename T> struct type : In<T,TT...> { };
// };
//
// template <template<typename...> class In, typename T1, typename... TT>
// struct bind_all_types_but_2 {
//   template <typename T> struct type : In<T1,T,TT...> { };
// };

// ******************************************************************

} // end namespace

#endif
