#ifndef IVANP_NEST_HH
#define IVANP_NEST_HH

template <template <typename...> class T, size_t N, typename Arg1, typename... Args>
struct nest {
  using type = T< typename nest<T,N-1,Arg1,Args...>::type, Args...>;
};

template <template <typename...> class T, typename Arg1, typename... Args>
struct nest<T,1,Arg1,Args...> {
  using type = T<Arg1,Args...>;
};

#endif
