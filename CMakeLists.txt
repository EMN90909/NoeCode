cmake_minimum_required(VERSION 3.16)
project(NoeCode VERSION 1.0.0 LANGUAGES CXX)

file(GLOB NOE_BOOTSTRAP_SOURCES CONFIGURE_DEPENDS compiler/bootstrap/*.cpp)

add_executable(noe ${NOE_BOOTSTRAP_SOURCES})
target_include_directories(noe PRIVATE include/noe)
target_compile_features(noe PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(noe PRIVATE /W4 /permissive-)
else()
  target_compile_options(noe PRIVATE -Wall -Wextra -pedantic)
endif()
