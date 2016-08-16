// Written by Ivan Pogrebnyak

#ifndef IVANP_IS_TEMPLATE_TYPE_TRAITS_HH
#define IVANP_IS_TEMPLATE_TYPE_TRAITS_HH

#include <type_traits>

#define DEFINE_IS_TYPE_TRAIT(TYPE,NAME) \
template <typename T> struct NAME##_impl { enum { value = 0 }; }; \
template <> struct NAME##_impl<TYPE> { enum { value = 1 }; }; \
template <typename T> struct NAME : NAME##_impl< \
  typename std::remove_reference<typename std::remove_cv<T>::type>::type> { };

#define DEFINE_IS_TEMPLATE_TRAIT(TYPE,NAME) \
template <typename T> struct NAME##_impl { enum { value = 0 }; }; \
template <typename... TT> struct NAME##_impl<TYPE<TT...>> \
{ enum { value = 1 }; }; \
template <typename T> struct NAME : NAME##_impl< \
  typename std::remove_reference<typename std::remove_cv<T>::type>::type> { };

#endif
