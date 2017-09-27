#include <iostream>
#include <string>
#include <exception>
#include <ostream>
#include <iterator>
#include <cctype>
#include <algorithm>
#include <vector>
#include <cctype>
#include <sstream>
#include <deque>
#include <ctime>
#include <thread>
#include <future>
#include <functional>
#include <fstream>
#include <cassert>
#include <random>

#include "bigint.hpp"

using namespace std;
int main() {
try{
  big_unsigned x, y;
  while(cin >> x >> y){
    auto start = std::chrono::system_clock::now();
    std::cout << "x+y = " << x+y << std::endl;
    std::cout << "x*y = " << (x*y).size() << std::endl;
    std::cout << "x-y = " << x-y << std::endl;
    auto end = std::chrono::system_clock::now();
    std::cerr << "total time: " << std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us" << std::endl;
  }
} catch(std::exception& e) {
  std::cerr << e.what() << std::endl;
}
  return 0;
}
