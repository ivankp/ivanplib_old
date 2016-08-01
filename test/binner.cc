// Written by Ivan Pogrebnyak

#include <iostream>
#include <vector>
#include <array>
#include <deque>
#include "test_marcos.hh"

#include "binner.hh"

using std::cout;
using std::endl;

using ivanp::binner;

template <typename T> struct vec_filler {
  void operator()(std::vector<T>& vec, T&& x) {
    vec.emplace_back(x);
  }
};

int main()
{
  binner<double> histD(5,0,5);

  histD.fill(0.5);
  histD.fill(1);
  histD(3,5);
  histD(4.1,3);
  histD(3.9);
  histD(5,9);
  histD(-1,7);

  for (unsigned i=0; i<histD.bins().size(); ++i)
    cout << "bin "<< i << " ["
         << histD.ledge(i) << ',' << histD.redge(i)
         << "): " << histD[i] << endl;

  cout << endl;
  cout << "Should be: 7 1 1 0 6 3 9" << endl;
  // **********************************

  binner<
    binner<int>,float,
    ivanp::default_bin_filler<binner<double>>,std::deque<float>
  > bb(3,0,3);
  for (auto& bin : bb.bins()) bin = {1,2};
  print_type(bb.bins().back())
  bb.bins().back() = {1,3,6,8};

  bb.fill(1,1);
  bb.fill(1,2);
  bb.fill(3.5,2);
  bb(3.5,7,3);

  for (unsigned i=0; i<bb.bins().size(); ++i)
    for (unsigned j=0; j<bb[i].bins().size(); ++j)
      cout << "bin "<< i<<','<<j << " ["
           << bb.ledge(i) << ',' << bb.redge(i) << ")["
           << bb[i].ledge(j) << ',' << bb[i].redge(j)
           << "): " << bb[i][j] << endl;
  // **********************************
  cout << endl;

  binner<
    std::vector<std::string>, unsigned, vec_filler<std::string>
  > strings_by_size(10,1,11);
  strings_by_size(5,"hello");
  strings_by_size(5,"world");

  for (unsigned i=0; i<strings_by_size.nbinso(); ++i) {
    cout << "bin "<< i << " ["
         << strings_by_size.ledge(i) << ',' << strings_by_size.redge(i)
         << "): ";
    for (const auto& str : strings_by_size[i])
      cout << str << ' ';
    cout << endl;
  }

  return 0;
}
