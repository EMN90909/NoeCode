cmake_minimum_required(VERSION 3.16)
project(noqeri VERSION 1.0.0 LANGUAGES CXX)

file(GLOB NOQERI_BOOTSTRAP_SOURCES CONFIGURE_DEPENDS Compiler/bootstrap/*.cpp)

add_executable(noqeri ${NOQERI_BOOTSTRAP_SOURCES})
target_include_directories(noqeri PRIVATE Include/noqeri Compiler/bootstrap)
target_compile_features(noqeri PRIVATE cxx_std_17)
target_compile_definitions(noqeri PRIVATE NOQERI_BOOTSTRAP=1)

if(MSVC)
  target_compile_options(noqeri PRIVATE /W4 /permissive- /EHsc)
else()
  target_compile_options(noqeri PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(NAME noqeri-version COMMAND noqeri --version)
  add_test(NAME noqeri-check-hello COMMAND noqeri check ${CMAKE_SOURCE_DIR}/examples/hello.nqr)
  add_test(NAME noqeri-tests COMMAND noqeri test ${CMAKE_SOURCE_DIR}/tests)
  add_test(NAME noqeri-doctor COMMAND noqeri doctor ${CMAKE_SOURCE_DIR})
endif()

install(TARGETS noqeri RUNTIME DESTINATION bin)
install(FILES Include/noqeri/noqeri.hpp DESTINATION include/noqeri)
