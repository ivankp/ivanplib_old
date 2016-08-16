// Written by Ivan Pogrebnyak

#ifndef IVANP_EMPLACE_TRAITS_HH
#define IVANP_EMPLACE_TRAITS_HH

#include <type_traits>

#include "expression_traits.hh"
#include "extra_traits.hh"

namespace ivanp {

DEFINE_BINARY_TRAIT(has_emplace_back, x1.emplace_back(x2));
DEFINE_BINARY_TRAIT(has_emplace, x1.emplace(x2));
DEFINE_BINARY_TRAIT(has_emplace_front, x1.emplace_front(x2));

template <typename T, typename = void>
struct can_emplace { enum { value = false }; };

template <typename T> struct can_emplace<T, enable_if_t<
  has_emplace_back<T, typename T::value_type>::value
>> {
  enum { value = true };
  using type = remove_args_const_t<typename T::value_type>;
  template <typename... Args>
  inline static void emplace(T* x, Args&&... args)
  noexcept(noexcept(x->emplace_back(std::forward<Args>(args)...)))
  {
    x->emplace_back(std::forward<Args>(args)...);
  }
};

template <typename T> struct can_emplace<T, enable_if_t<
  !has_emplace_back<T, typename T::value_type>::value &&
  has_emplace<T, typename T::value_type>::value
>> {
  enum { value = true };
  using type = remove_args_const_t<typename T::value_type>;
  template <typename... Args>
  inline static void emplace(T* x, Args&&... args)
  noexcept(noexcept(x->emplace(std::forward<Args>(args)...)))
  {
    x->emplace(std::forward<Args>(args)...);
  }
};

template <typename T> struct can_emplace<T, enable_if_t<
  !has_emplace_back<T, typename T::value_type>::value &&
  !has_emplace<T, typename T::value_type>::value &&
  has_emplace_front<T, typename T::value_type>::value
>> {
  enum { value = true };
  using type = remove_args_const_t<typename T::value_type>;
  template <typename... Args>
  inline static void emplace(T* x, Args&&... args)
  noexcept(noexcept(x->emplace_front(std::forward<Args>(args)...)))
  {
    x->emplace_front(std::forward<Args>(args)...);
  }
};

} // end namespace

#endif
