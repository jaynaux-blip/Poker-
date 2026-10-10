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
  # CMake's default flags turn exceptions and RTTI on; drop them so /EHs-c- and /GR- don't clash.
  string(REGEX REPLACE "/EHsc|/GR( |$)" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  target_compile_options(ShortStackCore PUBLIC /W4 /WX /fp:precise /EHs-c- /GR-)
  # Without _HAS_EXCEPTIONS=0, MSVC's STL still compiles try/catch and warns (C4530) under /EHs-c-.
  # The tests write files with std::fopen, which MSVC flags as deprecated (C4996).
  target_compile_definitions(ShortStackCore PUBLIC _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
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

# Where the goldens and the fixtures live; the next segment's suites and per-track files read them by this path.
set(SS_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../Tests)

# night_one_session and riverline_ui also compile the next segment's per-track files (next_segment_spec §7.6): each
# track adds its checks in its own file, called from the suite's main.
add_executable(session_test tests/session_main.cpp tests/session_seams.cpp
  tests/session_seg_stake.cpp tests/session_seg_sponsor.cpp tests/session_seg_tableui.cpp)
target_link_libraries(session_test PRIVATE ShortStackCore)
target_compile_definitions(session_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME night_one_session COMMAND session_test)

add_executable(ui_test tests/ui_main.cpp
  tests/ui_seg_stake.cpp tests/ui_seg_sponsor.cpp tests/ui_seg_tableui.cpp tests/ui_seg_badges.cpp tests/ui_seg_laptop.cpp)
target_link_libraries(ui_test PRIVATE ShortStackCore)
target_compile_definitions(ui_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME riverline_ui COMMAND ui_test)

add_executable(monkey_test tests/monkey_main.cpp)
target_link_libraries(monkey_test PRIVATE ShortStackCore)
add_test(NAME monkey_play COMMAND monkey_test)

add_executable(world_test tests/world_main.cpp)
target_link_libraries(world_test PRIVATE ShortStackCore)
add_test(NAME living_world COMMAND world_test)

add_executable(audio_test tests/audio_main.cpp)
target_link_libraries(audio_test PRIVATE ShortStackCore)
add_test(NAME synthesized_audio COMMAND audio_test)

# Northside (the realism update): the shops, the wardrobe, fitness and the pawn (core-shops); the shop screens
# (core-ui); the town, talk and the weather (people-core).
add_executable(shops_test tests/shops_main.cpp)
target_link_libraries(shops_test PRIVATE ShortStackCore)
add_test(NAME northside_shops COMMAND shops_test)

add_executable(shops_ui_test tests/shops_ui_main.cpp)
target_link_libraries(shops_ui_test PRIVATE ShortStackCore)
add_test(NAME northside_shops_ui COMMAND shops_ui_test)

add_executable(town_test tests/town_main.cpp)
target_link_libraries(town_test PRIVATE ShortStackCore)
add_test(NAME northside_town COMMAND town_test)

# The next segment (next_segment_spec §7.6): backing (core-stake), sponsors (core-sponsor), the room's voice (core-room)
# and pro badges (core-badges). Every suite also runs the copy lint (tests/CopyLint.h) over its inventories.
add_executable(stake_test tests/staking_main.cpp)
target_link_libraries(stake_test PRIVATE ShortStackCore)
target_compile_definitions(stake_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME staking_backing COMMAND stake_test)

add_executable(sponsor_test tests/sponsor_main.cpp)
target_link_libraries(sponsor_test PRIVATE ShortStackCore)
target_compile_definitions(sponsor_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME sponsor_deals COMMAND sponsor_test)

# RoomField (tests/room_field.cpp) is the lead's: live::DescribeField against both old copies of the field code.
add_executable(room_test tests/room_main.cpp tests/room_field.cpp)
target_link_libraries(room_test PRIVATE ShortStackCore)
target_compile_definitions(room_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME card_room COMMAND room_test)

add_executable(badges_test tests/badges_main.cpp)
target_link_libraries(badges_test PRIVATE ShortStackCore)
target_compile_definitions(badges_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME pro_badges COMMAND badges_test)

# Not a suite: the tool that wrote Tests/fixtures/ from c09bc31's code (next_segment_spec §7.5), and the transcript that
# compares a build's sessions with that code's. It uses only what the session had then, so it builds against either.
add_executable(fixture_tool tests/fixture_tool.cpp)
target_link_libraries(fixture_tool PRIVATE ShortStackCore)
