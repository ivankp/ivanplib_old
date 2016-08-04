// Written by Ivan Pogrebnyak

#ifndef IVANP_EXTRA_TRAITS_HH
#define IVANP_EXTRA_TRAITS_HH

#include <type_traits>

#define DEFINE_IS_TYPE_TRAIT(TYPE) \
template <typename T> struct is_##TYPE##_impl { enum { value = 0 }; }; \
template <> struct is_##TYPE##_impl<TYPE> { enum { value = 1 }; }; \
template <typename T> struct is_##TYPE : is_##TYPE##_impl< \
  typename std::remove_reference<typename std::remove_cv<T>::type>::type> { };

#define DEFINE_IS_TEMPLATE_TRAIT(TYPE) \
template <typename T> struct is_##TYPE##_impl { enum { value = 0 }; }; \
template <typename... TT> struct is_##TYPE##_impl<TYPE<TT...>> \
  { enum { value = 1 }; }; \
template <typename T> struct is_##TYPE : is_##TYPE##_impl< \
  typename std::remove_reference<typename std::remove_cv<T>::type>::type> { };

namespace ivanp {

// ******************************************************************

template < bool... > struct bool_sequence {};

template < bool... B >
using bool_and = std::is_same< bool_sequence< B... >,
                               bool_sequence< ( B || true )... > >;
template < bool... B >
using bool_or = std::integral_constant< bool, !bool_and< !B... >::value >;

template <template<typename> class Condition, typename... TT>
using all_are = bool_and<Condition<TT>::value...>;

// template <typename T, template<typename> class Condition, typename... TT>
// using enable_if_all_are_t = typename std::enable_if<
//   all_are< Condition, TT...>::value, T >::type;

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

} // end namespace

#endif
