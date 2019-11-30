
#include "../mmaptwo.hpp"
#include <iostream>
#include <cstdlib>
#include <iomanip>

int main(int argc, char **argv) {
  std::cout << "check bequeath stop: " << std::boolalpha
    << mmaptwo::check_bequeath_stop() << std::endl;
  return EXIT_SUCCESS;
}

