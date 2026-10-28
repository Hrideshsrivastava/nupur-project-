cmake_minimum_required(VERSION 3.14)
project(MetroRouteNavigator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
  add_compile_options(/utf-8 /W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Core library: everything except main(), shared by the app and the tests.
add_library(metro_core
  src/StringUtil.cpp
  src/Trie.cpp
  src/MetroGraph.cpp
  src/MetroData.cpp
  src/CrowdModel.cpp
  src/FareCalculator.cpp
  src/Route.cpp
  src/RouteFinder.cpp
  src/Recommender.cpp
  src/JourneyHistory.cpp
  src/Terminal.cpp
  src/UI.cpp
  src/Renderer.cpp
  src/Exporter.cpp
  src/App.cpp
)
target_include_directories(metro_core PUBLIC include)

add_executable(metro_navigator src/main.cpp)
target_link_libraries(metro_navigator PRIVATE metro_core)

enable_testing()
add_executable(metro_tests tests/test_main.cpp)
target_link_libraries(metro_tests PRIVATE metro_core)
add_test(NAME metro_tests COMMAND metro_tests)
