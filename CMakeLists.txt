cmake_minimum_required(VERSION 3.7)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(podrm LANGUAGES C CXX)

add_subdirectory(vendor)

find_package(fmt REQUIRED)

add_library(podrm-reflection INTERFACE)

target_compile_features(podrm-reflection INTERFACE cxx_std_20)
target_include_directories(podrm-reflection SYSTEM INTERFACE include)
target_link_libraries(podrm-reflection INTERFACE Boost::pfr)

add_subdirectory(lib/postgres)
add_subdirectory(lib/sqlite)

option(PFR_ORM_USE_GSL_SPAN
       "Use Microsoft.GSL for span implementation instead of std::span" OFF)
if(PFR_ORM_USE_GSL_SPAN)
  target_compile_definitions(podrm-reflection INTERFACE PFR_ORM_USE_GSL_SPAN)
  find_package(Microsoft.GSL REQUIRED)
  target_link_libraries(podrm-reflection INTERFACE Microsoft.GSL::GSL)
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  option(PFR_ORM_ASAN "Build podrm with address sanitizer" OFF)

  if(PFR_ORM_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
  endif()

  include(CTest)
  if(BUILD_TESTING)
    add_subdirectory(test)
  endif()
endif()
