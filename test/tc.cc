// Written by Ivan Pogrebnyak

#include <iostream>
#include <chrono>
#include <thread>

#include "timed_counter.hh"

using std::cout;
using std::endl;

using tc = ivanp::timed_counter<unsigned>;

int main(int argc, char const *argv[]) {

  for (tc i(500); i.ok(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}
