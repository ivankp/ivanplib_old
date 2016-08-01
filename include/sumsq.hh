// Written by Ivan Pogrebnyak

#ifndef IVANP_SUMS_OF_SQUARES_HH
#define IVANP_SUMS_OF_SQUARES_HH

#include <type_traits>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <array>

namespace ivanp {

// Type aliases -------------------------------------------

namespace detail { namespace sq {

  // bool types compositing

  template< bool... > struct bool_sequence {};

  template< bool... B >
  using bool_and = std::is_same< bool_sequence< B... >,
                                 bool_sequence< ( B || true )... > >;
  template< bool... B >
  using bool_or = std::integral_constant< bool, !bool_and< !B... >::value >;

  template <template<typename> class Condition, typename... TT>
  using all_are = bool_and<Condition<TT>::value...>;

  // SFINAE for arithmetic

  template< bool B, typename T = void >
  using enable_if_t = typename std::enable_if<B,T>::type;

  template <typename T, typename... A>
  using enable_if_arithmetic_t
    = enable_if_t<all_are<std::is_arithmetic,A...>::value,T>;

  template <typename T, typename... A>
  using enable_if_not_arithmetic_t
    = enable_if_t<!all_are<std::is_arithmetic,A...>::value,T>;

  template <typename... TT>
  using common_t = typename std::common_type<TT...>::type;

  template <typename... A>
  using enable_if_arithmetic_common_t
    = enable_if_arithmetic_t<common_t<A...>,A...>;

}}

// Arithmetic types ---------------------------------------
// pass by value

template <typename T>
[[ gnu::const, gnu::flatten ]]
inline detail::sq::enable_if_arithmetic_t<T,T>
sq(T x) noexcept { return x*x; }

template <typename T, typename... TT>
[[ gnu::const, gnu::flatten ]]
inline detail::sq::enable_if_arithmetic_common_t<T,TT...>
sq(T x, TT... xx) noexcept { return sq(x)+sq(xx...); }

template <typename T, typename... TT>
[[ gnu::const, gnu::flatten ]]
inline detail::sq::enable_if_arithmetic_common_t<T,TT...>
quad_sum(T x, TT... xx) noexcept { return std::sqrt(sq(x,xx...)); }

// std::vector --------------------------------------------

namespace detail { namespace sq {
  template <typename C1, typename C2>
  void containers_size_check(const C1& c1, const C2& c2) {
    if (c1.size()!=c2.size()) throw std::out_of_range(
      "containers of different size");
  }
}}

template <typename T, typename Out=T>
inline std::vector<Out> sq(const std::vector<T>& in) {
  std::vector<Out> tmp;
  tmp.reserve(in.size());
  for (const auto& x : in) tmp.emplace_back(sq(x));
  return std::move(tmp);
}

// std::array ---------------------------------------------

namespace detail { namespace sq {
  template<int ...> struct seq { };
  template<int N, int ...S> struct gens : gens<N-1, N-1, S...> { };
  template<int ...S> struct gens<0, S...> { typedef seq<S...> type; };
  template<int N> using gens_t = typename gens<N>::type;
  template<typename T> struct type_wrap { };

  template <size_t N, typename T, typename Out, int ...I>
  inline std::array<Out,N> sq_impl(const std::array<T,N>& in,
    type_wrap<Out>, seq<I...>)
  {
    return { (Out)::ivanp::sq(std::get<I>(in))... };
  }
}}

template <size_t N, typename T, typename Out=T>
inline std::array<Out,N> sq(const std::array<T,N>& in)
{
  return std::move( detail::sq::sq_impl( in,
    detail::sq::type_wrap<Out>(),
    detail::sq::gens_t<N>() ) );
}

// Add two containers -------------------------------------

template <typename T, typename In, typename Alloc1, typename Alloc2>
inline void add_sq(std::vector<T,Alloc1>& sum2,
                   const std::vector<In,Alloc2>& in)
{
  detail::sq::containers_size_check(sum2,in);
  for (size_t i=0, n=in.size(); i<n; ++i) sum2[i] += sq(in[i]);
}

template <size_t N, typename T, typename In>
inline detail::sq::enable_if_arithmetic_t<void,T,In>
add_sq(std::array<T,N>& sum2, const std::array<In,N>& in) {
  for (size_t i=0; i<N; ++i) sum2[i] += sq(in[i]);
}

template <size_t N, typename T, typename In, typename Alloc>
inline detail::sq::enable_if_arithmetic_t<void,T,In>
add_sq(std::array<T,N>& sum2, const std::vector<In,Alloc>& in) {
  detail::sq::containers_size_check(sum2,in);
  for (size_t i=0; i<N; ++i) sum2[i] += sq(in[i]);
}

template <typename T, typename Alloc, typename In, size_t N>
inline detail::sq::enable_if_arithmetic_t<void,T,In>
add_sq(std::vector<T,Alloc>& sum2, const std::array<In,N>& in) {
  detail::sq::containers_size_check(sum2,in);
  for (size_t i=0; i<N; ++i) sum2[i] += sq(in[i]);
}

// Variadic add_sq ----------------------------------------

namespace detail { namespace sq {
  template<typename T>
  struct is_std_vector: std::false_type { };

  template<typename T, typename Alloc>
  struct is_std_vector<std::vector<T,Alloc>>: std::true_type { };

  template<typename T>
  struct is_std_array: std::false_type { };

  template<typename T, size_t N>
  struct is_std_array<std::array<T,N>>: std::true_type { };
}}

// Add arithmetics to containers
template <typename Sum, typename... TT>
inline detail::sq::enable_if_t<
  detail::sq::all_are<std::is_arithmetic,TT...>::value &&
  ( detail::sq::is_std_vector<Sum>::value ||
    detail::sq::is_std_array<Sum>::value )
>
add_sq(Sum& sum, const TT&... xx) noexcept {
  auto arith_sq = sq(xx...);
  for (auto& x : sum) x += arith_sq;
}

// general add_sq
template <typename Sum, typename T, typename... TT>
inline detail::sq::enable_if_t<
  !detail::sq::all_are<std::is_arithmetic,T,TT...>::value &&
  ( detail::sq::is_std_vector<Sum>::value ||
    detail::sq::is_std_array<Sum>::value )
>
add_sq(Sum& sum, const T& x, const TT&... xx) {
  add_sq(sum,x);
  add_sq(sum,xx...);
}

// Variadic sq --------------------------------------------

namespace detail { namespace sq {

  template <typename T>
  struct data_type { using type = T; };

  template <typename T, typename Alloc>
  struct data_type<std::vector<T,Alloc>> { using type = T; };

  template <typename T, size_t N>
  struct data_type<std::array<T,N>> { using type = T; };

  template <typename... TT>
  using common_data_t = common_t<typename data_type<TT>::type...>;

}}

template <typename T, typename... TT>
inline detail::sq::enable_if_arithmetic_t<
  std::vector<detail::sq::common_data_t<T, TT...>>, T
>
sq(const std::vector<T>& in1, const TT&... in) {
  auto tmp = sq<T,detail::sq::common_data_t<T, TT...>>(in1);
  add_sq(tmp,in...);
  return std::move(tmp);
}

template <size_t N, typename T, typename... TT>
inline detail::sq::enable_if_arithmetic_t<
  std::array<detail::sq::common_data_t<T, TT...>,N>, T
>
sq(const std::array<T,N>& in1, const TT&... in) {
  auto tmp = sq<N,T,detail::sq::common_data_t<T, TT...>>(in1);
  add_sq(tmp,in...);
  return std::move(tmp);
}

// --------------------------------------------------------

template <typename T, typename... TT>
inline auto quad_sum(const T& x, const TT&... xx) noexcept
-> detail::sq::enable_if_t<
  detail::sq::is_std_vector<T>::value ||
  detail::sq::is_std_array<T>::value, decltype(sq(x,xx...)) >
{
  auto tmp = sq(x,xx...);
  for (auto& x : tmp) x = std::sqrt(x);
  return std::move(tmp);
}

// --------------------------------------------------------

} // end namespace

#endif
