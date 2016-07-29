#include <iostream>

#include "sumsq.hh"

#define test(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::endl;

using namespace goop;

int main(int argc, char const *argv[]) {

  test( sq(1,2,3) )
  test( sq(1,2.5,3) )

  int a = 5;
  int& b = a;
  test( sq(a,b,3) )

  test( sq(3.0,4.0) )
  test( quad_sum(3,4) )

  cout << endl;
  // ------------------------------------------------------

  std::vector<int> v1 {1,2,3};
  std::vector<unsigned> v2 {4,5,6};
  std::vector<double> v3 {-3.,-2.,-1.5};

  auto svs1 = sq(v1,v2);
  for (auto x : svs1) cout << ' ' << x;
  cout << endl << endl;

  auto svs2 = sq(v1,v2,v3);
  for (auto x : svs2) cout << ' ' << x;
  cout << endl << endl;
  // ------------------------------------------------------

  std::array<int,3> a1 {1,2,3};
  std::array<unsigned,3> a2 {4,5,6};
  std::array<double,3> a3 {-3.,-2.,-1.5};

  auto sas1 = sq(a1,a2);
  for (auto x : sas1) cout << ' ' << x;
  cout << endl << endl;

  auto sas2 = sq(a1,a2,a3);
  for (auto x : sas2) cout << ' ' << x;
  cout << endl;
  // ------------------------------------------------------

  return 0;
}
