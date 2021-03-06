cmake_minimum_required(VERSION 3.10.2) # Ubuntu 18.04 LTS Default

include(GNUInstallDirs)

# ------------------------------------------------------------------------------
#  Project Settings
# ------------------------------------------------------------------------------

execute_process(
  COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/VERSION
  COMMAND tr -d "\n"
  OUTPUT_VARIABLE CURRENT_VERSION)

project(meevax VERSION ${CURRENT_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os    -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O3 -g -DNDEBUG -pipe") # NOTE: -march=native causes "Illegal instruction" error (is Valgrind's bug) on CI.

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

find_package(Boost REQUIRED)
# find_package(Threads REQUIRED)


# ------------------------------------------------------------------------------
#  Configure README
# ------------------------------------------------------------------------------

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/configure/README.md)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configure/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
endif()


# ------------------------------------------------------------------------------
#  Configure Headers
# ------------------------------------------------------------------------------

string(TIMESTAMP ${PROJECT_NAME}_BUILD_DATE "%Y/%m/%d-%H:%M:%S")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
  execute_process(
    COMMAND git rev-parse HEAD
    COMMAND tr -d "\n"
    OUTPUT_VARIABLE ${PROJECT_NAME}_BUILD_HASH)
else()
  set(${PROJECT_NAME}_BUILD_HASH "unspecified")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/configure/feature.hpp)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configure/feature.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/kernel/feature.hpp)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/configure/version.hpp)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configure/version.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/kernel/version.hpp)
endif()


# ------------------------------------------------------------------------------
#  Convert Scheme Codes to ELF Binary
# ------------------------------------------------------------------------------

file(GLOB ${PROJECT_NAME}_BASIS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/basis/*.ss)

set(${PROJECT_NAME}_BASIS "")

foreach(EACH IN LISTS ${PROJECT_NAME}_BASIS_SOURCES)
  get_filename_component(FILENAME ${EACH} NAME)
  get_filename_component(FILEPATH ${EACH} PATH)

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.o
    WORKING_DIRECTORY ${FILEPATH}
    COMMAND ${CMAKE_OBJCOPY}
    ARGS -I binary
         -O elf64-x86-64
         -B i386
         ${FILENAME}
         ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.o)

  list(APPEND ${PROJECT_NAME}_BASIS ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.o)
endforeach()


# ------------------------------------------------------------------------------
#  Meevax Kernel
# ------------------------------------------------------------------------------

add_library(Kernel SHARED "")
add_library(Meevax::Kernel ALIAS Kernel)

file(GLOB_RECURSE ${PROJECT_NAME}_KERNEL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp)

target_sources(Kernel PRIVATE
  ${${PROJECT_NAME}_BASIS}
  ${${PROJECT_NAME}_KERNEL_SOURCES})

target_include_directories(Kernel PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(Kernel PRIVATE
  Boost::boost
  # Threads::Threads
  )

target_link_libraries(Kernel PUBLIC
  ${CMAKE_DL_LIBS}
  gmp
  stdc++fs)

set_target_properties(Kernel PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  OUTPUT_NAME ${PROJECT_NAME})

install(
  TARGETS Kernel
  EXPORT ${PROJECT_NAME}-config # Generate Configuration
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install( # Install generated config.cmake
  EXPORT ${PROJECT_NAME}-config
  EXPORT_LINK_INTERFACE_LIBRARIES
  NAMESPACE Meevax::
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


# ------------------------------------------------------------------------------
#   Meevax REPL
# ------------------------------------------------------------------------------

add_executable(REPL ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_link_libraries(REPL PRIVATE Meevax::Kernel)

set_target_properties(REPL PROPERTIES
  OUTPUT_NAME ${PROJECT_NAME})

install(
  TARGETS REPL
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


# ------------------------------------------------------------------------------
#  Meevax Native Library (DEPRECATED)
# ------------------------------------------------------------------------------

file(GLOB ${PROJECT_NAME}_BASIS_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.cpp)

foreach(EACH_SOURCE IN LISTS ${PROJECT_NAME}_BASIS_LIBRARY_SOURCES)
  string(REGEX REPLACE "^/(.*/)*(.*).cpp$" "meevax-\\2" TARGET_NAME ${EACH_SOURCE})

  add_library(${TARGET_NAME} SHARED ${EACH_SOURCE})

  set_target_properties(${TARGET_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

  target_link_libraries(${TARGET_NAME} PUBLIC Meevax::Kernel)

  install(
    TARGETS ${TARGET_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endforeach()


# ------------------------------------------------------------------------------
#  Package Version
# ------------------------------------------------------------------------------

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  COMPATIBILITY SameMajorVersion)

install( # Install Package Version File
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})


# ------------------------------------------------------------------------------
#  make uninstall
# ------------------------------------------------------------------------------

add_custom_target(uninstall
  # COMMAND xargs sudo rm -rfv < ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt
  COMMAND sudo rm -rf ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME}
  COMMAND sudo rm -rf ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  COMMAND sudo rm -rf ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}*
  COMMAND sudo rm -rf ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})


# ------------------------------------------------------------------------------
#  make test
# ------------------------------------------------------------------------------

enable_testing()

macro(define_test TEST_NAME)
  add_test(
    NAME ${TEST_NAME}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/meevax
            ${CMAKE_CURRENT_SOURCE_DIR}/test/${TEST_NAME}.ss)
endmacro()

define_test(abandoned)
define_test(chibi-basic)
define_test(low-level-macro-facility)
define_test(numerical-operations)
define_test(r4rs)
define_test(r4rs-appendix)
define_test(r7rs)
define_test(sicp-1)

add_test(
  NAME example
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/example/test.sh
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER})
