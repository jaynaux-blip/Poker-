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
# engine_invariants reads a day-0 fixture's checkpoint (update 2 SPEC §6.7: bda07f3's ck1 restores).
target_compile_definitions(unit_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")

# night_one_session and riverline_ui also compile the next segment's per-track files (next_segment_spec §7.6): each
# track adds its checks in its own file, called from the suite's main.
add_executable(session_test tests/session_main.cpp tests/session_seams.cpp
  tests/session_seg_stake.cpp tests/session_seg_sponsor.cpp tests/session_seg_tableui.cpp
  tests/session_seg_auto.cpp tests/session_seg_video.cpp tests/session_seg_sites.cpp tests/session_seg_econ.cpp)
target_link_libraries(session_test PRIVATE ShortStackCore)
target_compile_definitions(session_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME night_one_session COMMAND session_test)

add_executable(ui_test tests/ui_main.cpp
  tests/ui_seg_stake.cpp tests/ui_seg_sponsor.cpp tests/ui_seg_tableui.cpp tests/ui_seg_badges.cpp tests/ui_seg_laptop.cpp
  tests/ui_seg_auto.cpp tests/ui_seg_video.cpp tests/ui_seg_sites.cpp tests/ui_seg_creator.cpp tests/ui_seg_tableauto.cpp)
target_link_libraries(ui_test PRIVATE ShortStackCore)
target_compile_definitions(ui_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME riverline_ui COMMAND ui_test)

add_executable(monkey_test tests/monkey_main.cpp)
target_link_libraries(monkey_test PRIVATE ShortStackCore)
add_test(NAME monkey_play COMMAND monkey_test)

add_executable(world_test tests/world_main.cpp tests/world_seg_fresh.cpp tests/world_seg_sites.cpp)
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

# RoomField (tests/room_field.cpp) is the lead's: live::DescribeField against both old copies of the field code. Update 2
# (update 2 SPEC §6.7): core-autoplay's LiveSkipSliceEqualsTicks and RoomPace under the plan (tests/room_seg_auto.cpp).
add_executable(room_test tests/room_main.cpp tests/room_field.cpp tests/room_seg_auto.cpp)
target_link_libraries(room_test PRIVATE ShortStackCore)
target_compile_definitions(room_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
# Update 2, C1 (the lead's): the plan's rules at the card room, BackRoomAutoRules.h, a game header over the core's types alone
# (tests/room_seg_autorules.cpp: the read's direction, the last aggressor, the bluff line, the heart's terms, watch == skip
# with the read on). Only this target gets the game's source directory, for that one header. Its own executable for now (the
# file exposes room_seg_autorules::Run() for room_test's main()), run inside card_room after room_test: both must pass.
add_executable(room_autorules_test tests/room_seg_autorules.cpp)
target_link_libraries(room_autorules_test PRIVATE ShortStackCore)
target_include_directories(room_autorules_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../Source/ShortStack)
target_compile_definitions(room_autorules_test PRIVATE SS_AUTORULES_MAIN=1)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/card_room.cmake [=[
# Generated by CMakeLists.txt: card_room runs room_test, then room_autorules_test, and passes only if both do.
execute_process(COMMAND "${ROOM}" RESULT_VARIABLE RoomResult)
execute_process(COMMAND "${RULES}" RESULT_VARIABLE RulesResult)
if(NOT RoomResult EQUAL 0 OR NOT RulesResult EQUAL 0)
  message(FATAL_ERROR "card_room: room_test ${RoomResult}, room_autorules_test ${RulesResult}")
endif()
]=])
add_test(NAME card_room COMMAND ${CMAKE_COMMAND} -DROOM=$<TARGET_FILE:room_test> -DRULES=$<TARGET_FILE:room_autorules_test> -P ${CMAKE_CURRENT_BINARY_DIR}/card_room.cmake)

add_executable(badges_test tests/badges_main.cpp tests/badges_seg_fresh.cpp)
target_link_libraries(badges_test PRIVATE ShortStackCore)
target_compile_definitions(badges_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME pro_badges COMMAND badges_test)

# Update 2 (update 2 SPEC §6.7): autoplay's styles (core-styles), Rerun (core-video) and the rival sites (core-sites). The
# default run is each suite's short CI run (ai_styles <= 30 s, video_channel <= 20 s, rival_sites <= 30 s); the slow modes
# go behind an argument (style_test ring | sng | strength).
add_executable(style_test tests/styles_main.cpp)
target_link_libraries(style_test PRIVATE ShortStackCore)
target_compile_definitions(style_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME ai_styles COMMAND style_test)

add_executable(video_test tests/video_main.cpp)
target_link_libraries(video_test PRIVATE ShortStackCore)
target_compile_definitions(video_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME video_channel COMMAND video_test)

add_executable(sites_test tests/sites_main.cpp)
target_link_libraries(sites_test PRIVATE ShortStackCore)
target_compile_definitions(sites_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME rival_sites COMMAND sites_test)

# Not a suite: core-styles' chart compiler (update 2 SPEC §2.4.2, §2.4.4), Tests/charts/*.txt into AI/ChartTables.cpp.
add_executable(chartgen tools/chartgen/chartgen_main.cpp)
target_link_libraries(chartgen PRIVATE ShortStackCore)
target_compile_definitions(chartgen PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")

# Not a suite: the tool that wrote Tests/fixtures/ from c09bc31's code (next_segment_spec §7.5) and update 2's day-0
# fixtures from bda07f3's (write2, update 2 SPEC §6.5), and the transcript that compares a build's sessions with that
# code's. It uses only what the session had then, so it builds against either.
add_executable(fixture_tool tests/fixture_tool.cpp)
target_link_libraries(fixture_tool PRIVATE ShortStackCore)

# Not a suite either, and opt-in (-DSS_UNITY_PROBE=ON): the whole core pasted into one translation unit, in path order and
# in reverse, the way Unreal's unity blobs paste files together. A file's private names used through a namespace-scope
# using-directive leak into every file after it in a blob, so two such names that collide (TableHud.cpp's Margin and
# StoreCounter.cpp's, in Phase C) break the Unreal build only when adaptive unity happens to put the two files together.
# This compiles every arrangement at once. Compile only: nothing links or runs.
option(SS_UNITY_PROBE "Compile the core as one unity blob, both orders, to catch Unreal unity-build name collisions" OFF)
if(SS_UNITY_PROBE)
  set(PROBE_SOURCES ${CORE_SOURCES})
  list(SORT PROBE_SOURCES CASE INSENSITIVE)
  foreach(Order forward reverse)
    if(Order STREQUAL "reverse")
      list(REVERSE PROBE_SOURCES)
    endif()
    set(Blob "// Generated by CMakeLists.txt (SS_UNITY_PROBE): the core as one unity blob, ${Order}.\n")
    foreach(Source ${PROBE_SOURCES})
      string(APPEND Blob "#include \"${Source}\"\n")
    endforeach()
    # Written through configure_file so an unchanged blob keeps its timestamp (no rebuild on every configure).
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/unity_probe_${Order}.cpp.in "${Blob}")
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/unity_probe_${Order}.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/unity_probe_${Order}.cpp COPYONLY)
    add_library(unity_probe_${Order} OBJECT ${CMAKE_CURRENT_BINARY_DIR}/unity_probe_${Order}.cpp)
    target_include_directories(unity_probe_${Order} PRIVATE ${MODULE_DIR}/Public ${MODULE_DIR}/Private)
    # As Unreal 5.6 compiles the module: C++20, and on MSVC its conformance switches.
    set_target_properties(unity_probe_${Order} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    if(MSVC)
      target_compile_options(unity_probe_${Order} PRIVATE /W0 /bigobj /fp:precise /EHs-c- /GR- /permissive- /Zc:inline /Zc:preprocessor /Zc:strictStrings-)
      target_compile_definitions(unity_probe_${Order} PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
    else()
      target_compile_options(unity_probe_${Order} PRIVATE -w -ffp-contract=off -fno-exceptions -fno-rtti)
    endif()
  endforeach()
endif()
