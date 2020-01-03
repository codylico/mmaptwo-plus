
cmake_minimum_required(VERSION 3.0)

project(mmaptwo_plus CXX)

option(BUILD_TESTING "Enable testing.")
option(BUILD_SHARED_LIBS "Enable shared library construction.")
set(MMAPTWO_PLUS_OS CACHE STRING "Target memory mapping API.")

add_library(mmaptwo_plus "mmaptwo.cpp" "mmaptwo.hpp")
target_compile_features(mmaptwo_plus PRIVATE
    cxx_override cxx_nullptr cxx_constexpr cxx_noexcept
  )
if (MMAPTWO_PLUS_OS GREATER -1)
  target_compile_definitions(mmaptwo_plus
    PRIVATE "MMAPTWO_PLUS_OS=${MMAPTWO_PLUS_OS}")
endif (MMAPTWO_PLUS_OS GREATER -1)

if (BUILD_TESTING)
  add_executable(mmaptwo_plus_dump "tests/dump.cpp")
  target_link_libraries(mmaptwo_plus_dump mmaptwo_plus)

  add_executable(mmaptwo_plus_config "tests/config.cpp")
  target_link_libraries(mmaptwo_plus_config mmaptwo_plus)
endif (BUILD_TESTING)

