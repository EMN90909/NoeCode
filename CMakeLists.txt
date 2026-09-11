cmake_minimum_required(VERSION 3.16)
project(noqeri VERSION 1.0.0 LANGUAGES CXX)

option(NOQERI_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(NOQERI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(NOQERI_SOURCES
  Compiler/core.cpp
  Compiler/type_checker.cpp
  Compiler/nir.cpp
  Compiler/optimizer.cpp
  Compiler/native_backend.cpp
  Compiler/linker.cpp
  Parser/lexer.cpp
  Parser/parser.cpp
  Runtime/interpreter.cpp
  Programs/noqeri.cpp
  Programs/formatter.cpp
  Programs/package.cpp
  Programs/lsp.cpp
  Programs/test_runner.cpp
  Tools/build/production.cpp
)

add_executable(noqeri ${NOQERI_SOURCES})
target_include_directories(noqeri PRIVATE Include/noqeri Compiler/bootstrap)
target_compile_features(noqeri PRIVATE cxx_std_17)
target_compile_definitions(noqeri PRIVATE NOQERI_BOOTSTRAP=1)

if(MSVC)
  target_compile_options(noqeri PRIVATE /W4 /permissive- /EHsc)
  if(NOQERI_WARNINGS_AS_ERRORS)
    target_compile_options(noqeri PRIVATE /WX)
  endif()
else()
  target_compile_options(noqeri PRIVATE -Wall -Wextra -Wpedantic)
  if(NOQERI_WARNINGS_AS_ERRORS)
    target_compile_options(noqeri PRIVATE -Werror)
  endif()
  if(NOQERI_ENABLE_SANITIZERS)
    target_compile_options(noqeri PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(noqeri PRIVATE -fsanitize=address,undefined)
  endif()
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(NAME noqeri-version COMMAND noqeri --version)
  add_test(NAME noqeri-check-hello COMMAND noqeri check ${CMAKE_SOURCE_DIR}/examples/hello.nqr)
  add_test(NAME noqeri-tests COMMAND noqeri test ${CMAKE_SOURCE_DIR}/tests)
  add_test(NAME noqeri-doctor COMMAND noqeri doctor ${CMAKE_SOURCE_DIR})
endif()

include(GNUInstallDirs)
install(TARGETS noqeri RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES Include/noqeri/noqeri.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/noqeri)
install(DIRECTORY Lib/std/ DESTINATION ${CMAKE_INSTALL_DATADIR}/noqeri/std FILES_MATCHING PATTERN "*.nqr")
install(FILES Brand/noqeri-logo.webp Brand/noqeri-mark.png DESTINATION ${CMAKE_INSTALL_DATADIR}/noqeri/brand)
