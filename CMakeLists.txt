# copyright Nils Deppe 2018
# (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.1.3)

project(FlameFraphFilter VERSION 0.1 LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE OFF)

# Set the default build type to Release
if (NOT CMAKE_BUILD_TYPE)
  set(
    CMAKE_BUILD_TYPE Release
    CACHE STRING "Choose the type of build."
    FORCE
    )
endif()

# Use Boost ProgramOptions for parsing command line arguments
find_package(Boost 1.56.0 REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIR})
message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

set(EXECUTABLE flamegraph_filter)

set(EXECUTABLE_SOURCES
  flamegraph_filter.cpp
  )

add_executable(
  ${EXECUTABLE}
  ${EXECUTABLE_SOURCES}
  )

target_link_libraries(
  ${EXECUTABLE}
  ${Boost_LIBRARIES}
  )

set_property(
  TARGET ${EXECUTABLE}
  PROPERTY
  CXX_STANDARD 11
  )
