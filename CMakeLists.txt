cmake_minimum_required(VERSION 3.16)
project(noqeri VERSION 1.2.0 LANGUAGES CXX)
include(GNUInstallDirs)

option(NOQERI_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(NOQERI_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(NOQERI_FRONTEND_SOURCES
  Compiler/core.cpp
  Compiler/type_checker.cpp
  Compiler/nir.cpp
  Compiler/optimizer.cpp
  Parser/lexer.cpp
  Parser/parser.cpp
  Runtime/abi.cpp
  Runtime/interpreter.cpp
)

set(NOQERI_SOURCES
  ${NOQERI_FRONTEND_SOURCES}
  Compiler/native_backend.cpp
  Compiler/linker.cpp
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

add_library(noqeri_abi SHARED ${NOQERI_FRONTEND_SOURCES})
target_include_directories(noqeri_abi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include/noqeri>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/noqeri>
  PRIVATE Compiler/bootstrap)
target_compile_features(noqeri_abi PUBLIC cxx_std_17)
target_compile_definitions(noqeri_abi PRIVATE NOQERI_BOOTSTRAP=1)
set_target_properties(noqeri_abi PROPERTIES OUTPUT_NAME noqeri-abi WINDOWS_EXPORT_ALL_SYMBOLS ON)

function(noqeri_apply_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc)
    if(NOQERI_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    if(NOQERI_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
    if(NOQERI_ENABLE_SANITIZERS)
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
  endif()
endfunction()

noqeri_apply_warnings(noqeri)
noqeri_apply_warnings(noqeri_abi)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME noqeri-version COMMAND noqeri --version)
  add_test(NAME noqeri-check-hello COMMAND noqeri check ${CMAKE_SOURCE_DIR}/examples/hello.nqr)
  add_test(NAME noqeri-tests COMMAND noqeri test ${CMAKE_SOURCE_DIR}/tests)
  add_test(NAME noqeri-freestanding-build COMMAND noqeri build ${CMAKE_SOURCE_DIR}/examples/native_hello.nqr ${CMAKE_BINARY_DIR}/native_hello.s)
  add_test(NAME noqeri-doctor COMMAND noqeri doctor ${CMAKE_SOURCE_DIR})
endif()

install(TARGETS noqeri noqeri_abi
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES Include/noqeri/noqeri.hpp Include/noqeri/abi.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/noqeri)
install(DIRECTORY Lib/std/ DESTINATION ${CMAKE_INSTALL_DATADIR}/noqeri/std FILES_MATCHING PATTERN "*.nqr")
install(FILES Brand/noqeri-logo.webp Brand/noqeri-mark.png DESTINATION ${CMAKE_INSTALL_DATADIR}/noqeri/brand)
