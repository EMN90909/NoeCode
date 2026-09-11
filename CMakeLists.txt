cmake_minimum_required(VERSION 3.16)
project(Ric VERSION 1.0.0 LANGUAGES CXX)

file(GLOB RIC_BOOTSTRAP_SOURCES CONFIGURE_DEPENDS compiler/bootstrap/*.cpp)

add_executable(ric ${RIC_BOOTSTRAP_SOURCES})
target_include_directories(ric PRIVATE Include/ric compiler/bootstrap)
target_compile_features(ric PRIVATE cxx_std_17)
target_compile_definitions(ric PRIVATE RIC_BOOTSTRAP=1)

if(MSVC)
  target_compile_options(ric PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(ric PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(NAME ric-version COMMAND ric --version)
  add_test(NAME ric-check-hello COMMAND ric check ${CMAKE_SOURCE_DIR}/examples/hello.ric)
  add_test(NAME ric-tests COMMAND ric test ${CMAKE_SOURCE_DIR}/tests)
endif()

install(TARGETS ric RUNTIME DESTINATION bin)
install(FILES Include/ric/ric.hpp DESTINATION include/ric)
