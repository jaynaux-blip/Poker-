# Standalone build of ShortStackCore (no Unreal needed) for fast iteration and CI.
#   cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
cmake_minimum_required(VERSION 3.16)
project(ShortStackCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Source/ShortStackCore)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS ${MODULE_DIR}/Private/ShortStack/*.cpp)

add_library(ShortStackCore STATIC ${CORE_SOURCES})
target_include_directories(ShortStackCore PUBLIC ${MODULE_DIR}/Public)

# Mirror Unreal's constraints (no exceptions, no RTTI) and keep floating point
# bit-identical to the TypeScript build (no fused multiply-add).
if(MSVC)
  target_compile_options(ShortStackCore PUBLIC /W4 /WX /fp:precise /EHs-c- /GR-)
else()
  target_compile_options(ShortStackCore PUBLIC -Wall -Wextra -Wshadow -Wconversion -Werror
    -ffp-contract=off -fno-exceptions -fno-rtti)
endif()

add_executable(golden_test tests/golden_main.cpp)
target_link_libraries(golden_test PRIVATE ShortStackCore)

enable_testing()
add_test(NAME golden_vectors
  COMMAND golden_test ${CMAKE_CURRENT_SOURCE_DIR}/../Tests/golden_vectors.txt)

add_executable(unit_test tests/unit_main.cpp)
target_link_libraries(unit_test PRIVATE ShortStackCore)
add_test(NAME engine_invariants COMMAND unit_test)

add_executable(session_test tests/session_main.cpp)
target_link_libraries(session_test PRIVATE ShortStackCore)
add_test(NAME night_one_session COMMAND session_test)
