// Written by Ivan Pogrebnyak

#ifndef GOOP_SUMS_OF_SQUARES_HH
#define GOOP_SUMS_OF_SQUARES_HH

#include <type_traits>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <array>

namespace goop {

// Type aliases -------------------------------------------

template <typename... TT>
using common_t = typename std::common_type<TT...>::type;

template <typename A, typename T=A>
using enable_arithmetic_t
  = typename std::enable_if<std::is_arithmetic<A>::value,T>::type;

template <typename A, typename T=A>
using enable_not_arithmetic_t
  = typename std::enable_if<!std::is_arithmetic<A>::value,T>::type;

template <typename T, typename... TT>
using enable_arithmetic_common_t = enable_arithmetic_t<T, common_t<T, TT...>>;

// Arithmetic types ---------------------------------------
// pass by value

template <typename T>
inline enable_arithmetic_t<T> sq(T x) noexcept __attribute__((const, flatten));

template <typename T>
inline enable_arithmetic_t<T> sq(T x) noexcept { return x*x; }

template <typename T, typename... TT>
inline enable_arithmetic_common_t<T,TT...>
sq(T x, TT... xx) noexcept __attribute__((const, flatten));

template <typename T, typename... TT>
inline enable_arithmetic_common_t<T,TT...>
sq(T x, TT... xx) noexcept { return sq(x)+sq(xx...); }

template <typename T, typename... TT>
inline enable_arithmetic_common_t<T,TT...>
quad_sum(T x, TT... xx) noexcept __attribute__((const, flatten));

template <typename T, typename... TT>
inline enable_arithmetic_common_t<T,TT...>
quad_sum(T x, TT... xx) noexcept { return std::sqrt(sq(x,xx...)); }

// Non-arithmetic types -----------------------------------
// pass by reference
// move results

template <typename T>
inline enable_not_arithmetic_t<T> sq(const T& x) { return std::move(x*x); }

template <typename T, typename... TT>
inline enable_not_arithmetic_t<T, common_t<T, TT...>>
sq(const T& x, const TT&... xx) {
  return std::move(sq(x)+sq(xx...));
}

template <typename T, typename... TT>
inline enable_not_arithmetic_t<T, common_t<T, TT...>>
quad_sum(const T& x, const TT&... xx) { return std::sqrt(sq(x,xx...)); }

// std::vector --------------------------------------------

template <typename T1, typename T2>
void vectors_size_check(const std::vector<T1>& v1, const std::vector<T2>& v2) {
  if (v1.size()!=v2.size()) throw std::out_of_range(
    "vectors of different size");
}

template <typename Sum, typename In>
inline void add_sq(std::vector<Sum>& sum2, const std::vector<In>& in) {
  vectors_size_check(sum2,in);
  for (size_t i=0, n=in.size(); i<n; ++i) sum2[i] += sq(in[i]);
}

template <typename Sum, typename In1, typename... In>
inline void add_sq(std::vector<Sum>& sum2,
  const std::vector<In1>& in1, const std::vector<In>&... in)
{
  add_sq(sum2,in1);
  add_sq(sum2,in...);
}

template <typename T, typename Out=T>
inline std::vector<Out> sq(const std::vector<T>& in) {
  std::vector<Out> tmp;
  tmp.reserve(in.size());
  for (const auto& x : in) tmp.emplace_back(sq(x));
  return std::move(tmp);
}

template <typename T, typename... TT>
inline std::vector<common_t<T, TT...>>
sq(const std::vector<T>& in1, const std::vector<TT>&... in) {
  auto tmp = sq<T,common_t<T, TT...>>(in1);
  add_sq(tmp,in...);
  return std::move(tmp);
}

// std::array ---------------------------------------------

namespace detail_sq_std_array {
  template<int ...> struct seq { };
  template<int N, int ...S> struct gens : gens<N-1, N-1, S...> { };
  template<int ...S> struct gens<0, S...> { typedef seq<S...> type; };
  template<int N> using gens_t = typename gens<N>::type;
  template<typename T> struct type_wrap { };

  template <size_t N, typename T, typename Out, int ...I>
  inline std::array<Out,N> sq_impl(const std::array<T,N>& in,
    type_wrap<Out>, seq<I...>)
  {
    return { (Out)sq(std::get<I>(in))... };
  }
}

template <size_t N, typename Sum, typename In>
inline void add_sq(std::array<Sum,N>& sum2, const std::array<In,N>& in) {
  for (size_t i=0; i<N; ++i) sum2[i] += sq(in[i]);
}

template <size_t N, typename Sum, typename In1, typename... In>
inline void add_sq(std::array<Sum,N>& sum2,
  const std::array<In1,N>& in1, const std::array<In,N>&... in)
{
  add_sq(sum2,in1);
  add_sq(sum2,in...);
}

template <size_t N, typename T, typename Out=T>
inline std::array<Out,N> sq(const std::array<T,N>& in)
{
  return std::move( detail_sq_std_array::sq_impl( in,
    detail_sq_std_array::type_wrap<Out>(),
    detail_sq_std_array::gens_t<N>() ) );
}

template <size_t N, typename T, typename... TT>
inline std::array<common_t<T, TT...>,N>
sq(const std::array<T,N>& in1, const std::array<TT,N>&... in) {
  auto tmp = sq<N,T,common_t<T, TT...>>(in1);
  add_sq(tmp,in...);
  return std::move(tmp);
}

} // end namespace

#endif
