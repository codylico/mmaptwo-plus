
#include "../mmaptwo.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <cctype>
#include <iomanip>

int main(int argc, char **argv) {
  mmaptwo::mmaptwo_i* mi;
  mmaptwo::page_i* pager;
  char const* fname;
  if (argc < 5) {
    std::cerr << "usage: dump (file) (mode) (length) (offset)" << std::endl;
    return EXIT_FAILURE;
  }
  fname = argv[1];
  try {
    mi = mmaptwo::open(fname, argv[2],
      (size_t)std::strtoul(argv[3],nullptr,0),
      (size_t)std::strtoul(argv[4],nullptr,0));
  } catch (std::exception const& e) {
    std::cerr << "failed to open file '" << fname << "':" << std::endl;
    std::cerr << "\t" << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  try {
    pager = mi->acquire(mi->length(), 0);
  } catch (std::exception const& e) {
    delete mi;
    std::cerr << "failed to map file '" << fname << "':" << std::endl;
    std::cerr << "\t" << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  /* output the data */{
    size_t len = pager->length();
    unsigned char* bytes = (unsigned char*)pager->get();
    if (bytes != NULL) {
      size_t i;
      if (len >= std::numeric_limits<size_t>::max()-32)
        len = std::numeric_limits<size_t>::max()-32;
      for (i = 0; i < len; i+=16) {
        size_t j = 0;
        if (i)
          std::cout << std::endl;
        std::cout << std::setw(4) << std::setbase(16) << i << ':';
        for (j = 0; j < 16; ++j) {
          if (j%4 == 0) {
            std::cout << " ";
          }
          if (j < len-i)
            std::cout << std::setw(2) << std::setbase(16) << std::setfill('0')
              << (unsigned int)(bytes[i+j]);
          else std::cout << "  ";
        }
        std::cout << " | ";
        for (j = 0; j < 16; ++j) {
          if (j < len-i) {
            char ch = static_cast<char>(bytes[i+j]);
            std::cout << (isprint(ch) ? ch : '.');
          } else std::cout << ' ';
        }
      }
      std::cout << std::endl;
    } else {
      std::cerr << "mapped file '" << fname <<
        "' gives no bytes?" << std::endl;
    }
  }
  delete pager;
  delete mi;
  return EXIT_SUCCESS;
}

