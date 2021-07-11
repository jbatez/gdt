cmake_minimum_required(VERSION 3.15) # For CMP0092

project(gdt CXX)
include(CTest)

add_library(gdt INTERFACE)
target_compile_features(gdt INTERFACE cxx_std_20)
target_include_directories(gdt INTERFACE include)

if(BUILD_TESTING)
  list(APPEND test_names allocator)
  list(APPEND test_names assert)
  list(APPEND test_names assume)
  list(APPEND test_names dynarr)
  list(APPEND test_names panic)
  list(APPEND test_names unreachable)
  list(APPEND test_names vec)

  set(test_sources ${test_names})
  list(TRANSFORM test_sources APPEND .cxx)
  list(TRANSFORM test_sources PREPEND test/)
  create_test_sourcelist(test_sources test_driver.cxx ${test_sources})

  add_executable(test_driver ${test_sources})
  set_property(TARGET test_driver PROPERTY CXX_EXTENSIONS OFF)
  set_property(TARGET test_driver PROPERTY CXX_STANDARD 20)
  set_property(TARGET test_driver PROPERTY CXX_STANDARD_REQUIRED ON)
  target_link_libraries(test_driver gdt)

  if(MSVC)
    target_compile_options(test_driver PRIVATE /W4 /WX)
  else()
    target_compile_options(test_driver PRIVATE -Wpedantic -Wall -Wextra -Werror)
  endif()

  foreach(name ${test_names})
    add_test(NAME test_${name} COMMAND test_driver test/${name})
  endforeach()
endif()
