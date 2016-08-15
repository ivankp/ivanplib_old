#ifndef IVANP_TEST_CLASS_HH
#define IVANP_TEST_CLASS_HH

struct foo {
  static unsigned n;
  unsigned i;

  std::string s;
  foo() { std::cout << "default " << (i=(n++)) << std::endl; }
  foo(foo&& f): s(std::move(f.s)) {
    std::cout << "move " << (i=(n++)) << ' ' << s << std::endl;
  }
  foo(const foo& f): s(f.s) {
    std::cout << "copy " << (i=(n++)) << ' ' << s << std::endl;
  }
  foo& operator=(const foo& f) {
    s = f.s;
    std::cout << "copy assign " << i << ' ' << s << std::endl;
    return *this;
  }
  foo& operator=(foo&& f) {
    s = std::move(f.s);
    std::cout << "move assign " << i << ' ' << s << std::endl;
    return *this;
  }
  template <typename T, typename = typename std::enable_if<
    !std::is_same<
      typename std::remove_reference<
        typename std::remove_cv<T>::type
      >::type,
    foo>::value
  >::type>
  foo(T&& x): s(std::forward<T>(x)) {
    std::cout << "construct " << (i=(n++)) << ' ' << s << std::endl;
  }
  template <typename... Args, typename = typename std::enable_if<
    sizeof...(Args)!=1
  >::type>
  foo(Args&&... args): s(std::forward<Args>(args)...) {
    std::cout << "construct... " << (i=(n++)) << ' ' << s << std::endl;
  }
  template <typename T>
  typename std::enable_if<
    !std::is_same<
      typename std::remove_reference<
        typename std::remove_cv<T>::type
      >::type,
    foo>::value, foo
  >::type&
  operator=(T&& x) {
    s = std::forward<T>(x);
    std::cout << "assign " << i << ' ' << s << std::endl;
    return *this;
  }
  ~foo() { std::cout << "delete " << i << ' ' << s << std::endl; }
};
unsigned foo::n = 0;

#endif
