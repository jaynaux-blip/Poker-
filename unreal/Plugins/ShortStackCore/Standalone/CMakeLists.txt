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
# Update 4 G1 (the lead's): the festival careers start from Tests/fixtures/update4_festival.ssave.
target_compile_definitions(monkey_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
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

# Update 3 (update 3 SPEC §7.7, §8.2): video_main.cpp's title and copy checks moved verbatim to video_seg_copy.cpp
# (core-videocontent's from Phase A); the kit they share is tests/VideoSeg.h.
add_executable(video_test tests/video_main.cpp tests/video_seg_copy.cpp)
target_link_libraries(video_test PRIVATE ShortStackCore)
target_compile_definitions(video_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME video_channel COMMAND video_test)

add_executable(sites_test tests/sites_main.cpp)
target_link_libraries(sites_test PRIVATE ShortStackCore)
target_compile_definitions(sites_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME rival_sites COMMAND sites_test)

# Update 3 (update 3 SPEC §7.7): five suites of its own. Each has a short default run and its slow modes behind an argument.
# Budgets: site_play <= 30 s, rerun_algo <= 30 s, rerun_content <= 20 s, rivals_ui <= 60 s, rerun_ui <= 60 s. Three core:
# the revamped sites' loops, the Governor and FUSE (core-siteplay, with core-fuse's siteplay_fuse.cpp); Rerun 2.0's model
# and money (core-videoalgo); its titles, thumbnails, ideas and comments (core-videocontent). Two UI, apart from
# riverline_ui (64 s of its 75 s already): the rival sites' screens (ui-ignite, ui-rit) and Rerun 2.0's (ui-rerun,
# ui-thumbs), each track's checks in its own file, the kit's definitions in ui_seg_kit.cpp.
add_executable(siteplay_test tests/siteplay_main.cpp tests/siteplay_session.cpp tests/siteplay_fuse.cpp)
target_link_libraries(siteplay_test PRIVATE ShortStackCore)
target_compile_definitions(siteplay_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME site_play COMMAND siteplay_test)

add_executable(rerun_algo_test tests/rerun_algo_main.cpp)
target_link_libraries(rerun_algo_test PRIVATE ShortStackCore)
target_compile_definitions(rerun_algo_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME rerun_algo COMMAND rerun_algo_test)

add_executable(rerun_content_test tests/rerun_content_main.cpp)
target_link_libraries(rerun_content_test PRIVATE ShortStackCore)
target_compile_definitions(rerun_content_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME rerun_content COMMAND rerun_content_test)

add_executable(rivals_ui_test tests/rivals_ui_main.cpp tests/ui_seg_kit.cpp tests/ui_seg_seams.cpp tests/ui_seg_ignite.cpp tests/ui_seg_rit.cpp)
target_link_libraries(rivals_ui_test PRIVATE ShortStackCore)
target_compile_definitions(rivals_ui_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME rivals_ui COMMAND rivals_ui_test)

add_executable(rerun_ui_test tests/rerun_ui_main.cpp tests/ui_seg_kit.cpp tests/ui_seg_canvas.cpp tests/ui_seg_video3.cpp tests/ui_seg_thumbs.cpp)
target_link_libraries(rerun_ui_test PRIVATE ShortStackCore)
target_compile_definitions(rerun_ui_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME rerun_ui COMMAND rerun_ui_test)

# The creator update (SPEC_CREATORS §9.1, Update 3's G3 expanded): two suites of its own. kast_growth (the 23rd; <= 60 s, "long"
# the gates' run) links the core like video_channel: Kast's pure growth model (core-kastmath, kastgrowth_main.cpp), the Room and
# the careers on AdvanceTo's minute grid (core-kastroom, kastgrowth_session.cpp), the chat games (core-kastplay,
# kastgrowth_play.cpp); the kit is KastGrowthSeg.h. kast_ui (the 24th; <= 60 s) is Kast's screens (ui-kast, ui_seg_kast.cpp)
# apart from riverline_ui, with the UI kit's definitions in ui_seg_kit.cpp. The core's new files (KastGrowth.cpp, KastCopy.cpp,
# KastPlay.cpp, SessionKast.cpp, UI/RiverLineKastGrowth.cpp) come in through CORE_SOURCES' glob, as Unreal's module globs them.
add_executable(kastgrowth_test tests/kastgrowth_main.cpp tests/kastgrowth_session.cpp tests/kastgrowth_play.cpp)
target_link_libraries(kastgrowth_test PRIVATE ShortStackCore)
target_compile_definitions(kastgrowth_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME kast_growth COMMAND kastgrowth_test)

add_executable(kast_ui_test tests/kast_ui_main.cpp tests/ui_seg_kit.cpp tests/ui_seg_kast.cpp)
target_link_libraries(kast_ui_test PRIVATE ShortStackCore)
target_compile_definitions(kast_ui_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME kast_ui COMMAND kast_ui_test)

# Update 4 (update 4 SPEC v1.7 §6 PA-48, §8.1): four suites of its own, registered in Phase A with empty mains; each track's checks
# go in its own file, called from its suite's main. Caps (each suite's default run alone, its spec budget): festivals <= 30 s
# (Festival Season's core: festivals_main.cpp F-net, festivals_world.cpp F-world), world_u4 <= 45 s (the world at Update 4's scale:
# world_u4_main.cpp the lead's and A3's slice harness, _scene W-arrive, _growth W-growth, _story W-story, u4_session.cpp W-table
# then S-crowd, _crowd S-crowd, _promote S-promote, _scale S-scale; slow modes long, big3k and big outside ctest), big_field <= 30 s
# (bigfield_main.cpp B-engine, bigfield_session.cpp B-session; slow mode long) and u4_ui <= 60 s (Update 4's screens apart from
# riverline_ui and rivals_ui: u4_ui_main.cpp the kit, _fest F-hubs, _look F-look, _world W-ui, _bigfield B-ui, _crowd S-ui). The
# core's new files (Festival.cpp, NetworkFestival.cpp and the rest) come in through CORE_SOURCES' glob, as Unreal's module globs them.
add_executable(festivals_test tests/festivals_main.cpp tests/festivals_world.cpp)
target_link_libraries(festivals_test PRIVATE ShortStackCore)
target_compile_definitions(festivals_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME festivals COMMAND festivals_test)

add_executable(world_u4_test tests/world_u4_main.cpp tests/world_u4_scene.cpp tests/world_u4_growth.cpp tests/world_u4_story.cpp
  tests/world_u4_crowd.cpp tests/world_u4_promote.cpp tests/world_u4_scale.cpp tests/u4_session.cpp)
target_link_libraries(world_u4_test PRIVATE ShortStackCore)
target_compile_definitions(world_u4_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME world_u4 COMMAND world_u4_test)

add_executable(big_field_test tests/bigfield_main.cpp tests/bigfield_session.cpp)
target_link_libraries(big_field_test PRIVATE ShortStackCore)
target_compile_definitions(big_field_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")
add_test(NAME big_field COMMAND big_field_test)

# G1 (the lead's pre-wave commit H0; LEAD_CALLS R-1, R-2): u4_ui gains F-hubs' second stream (u4_ui_fest_hosts.cpp, Option B) and
# the UI kit (ui_seg_kit.cpp: UiSeg.h's page helpers), its three counters defined by u4_ui_main.cpp (SS_UI_KIT_MAIN_COUNTERS).
add_executable(u4_ui_test tests/u4_ui_main.cpp tests/u4_ui_fest.cpp tests/u4_ui_look.cpp tests/u4_ui_world.cpp tests/u4_ui_bigfield.cpp
  tests/u4_ui_crowd.cpp tests/u4_ui_fest_hosts.cpp tests/ui_seg_kit.cpp)
target_link_libraries(u4_ui_test PRIVATE ShortStackCore)
target_compile_definitions(u4_ui_test PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}" SS_UI_KIT_MAIN_COUNTERS=1)
add_test(NAME u4_ui COMMAND u4_ui_test)

# Update 4 Phase 0 (update 4 SPEC §6.1, §8.1; the lead's call P-4, durable/update4/LEAD_CALLS.md): every suite has a real
# TIMEOUT, max(its spec budget, 2.0 x its median run alone, 10 s), rounded up to a whole second. The spec budget is SPEC §8.1's
# cap for the four suites it lists (night_one_session 125 s, riverline_ui 75 s, rivals_ui 60 s, living_world 45 s), otherwise
# the default-run budget the suite's own spec set: monkey_play 120 s and pro_badges 90 s (update 2 SPEC §6.7), and those in the
# comments above (ai_styles 30 s, video_channel 20 s, rival_sites 30 s, site_play 30 s, rerun_algo 30 s, rerun_content 20 s,
# rerun_ui 60 s, kast_growth 60 s, kast_ui 60 s); the other nine suites have none. The medians are Phase 0's, three runs of each
# suite alone on a quiet PC at 8db6d21 (night_one_session 115.9 s, riverline_ui 69.1 s, monkey_play 62.7 s, pro_badges 47.8 s,
# rivals_ui 39.9 s, living_world 35.6 s); every one is within its budget. 2.0x, not 1.5x: under the build's own -j 3 a suite can
# take about 1.5x its time alone. The 10 s floor keeps a sub-second suite from failing on a slow start of its process. A suite
# that times out fails. Nobody raises a TIMEOUT without the lead; a suite whose run alone grows past its budget moves checks
# into its slow mode or splits.
set_tests_properties(golden_vectors PROPERTIES TIMEOUT 10)
set_tests_properties(engine_invariants PROPERTIES TIMEOUT 10)
set_tests_properties(night_one_session PROPERTIES TIMEOUT 232)
set_tests_properties(riverline_ui PROPERTIES TIMEOUT 139)
set_tests_properties(monkey_play PROPERTIES TIMEOUT 135)
set_tests_properties(living_world PROPERTIES TIMEOUT 72)
set_tests_properties(synthesized_audio PROPERTIES TIMEOUT 11)
set_tests_properties(northside_shops PROPERTIES TIMEOUT 35)
set_tests_properties(northside_shops_ui PROPERTIES TIMEOUT 24)
set_tests_properties(northside_town PROPERTIES TIMEOUT 23)
set_tests_properties(staking_backing PROPERTIES TIMEOUT 10)
set_tests_properties(sponsor_deals PROPERTIES TIMEOUT 10)
set_tests_properties(card_room PROPERTIES TIMEOUT 17)
set_tests_properties(pro_badges PROPERTIES TIMEOUT 110)
set_tests_properties(ai_styles PROPERTIES TIMEOUT 30)
set_tests_properties(video_channel PROPERTIES TIMEOUT 20)
set_tests_properties(rival_sites PROPERTIES TIMEOUT 30)
set_tests_properties(site_play PROPERTIES TIMEOUT 30)
set_tests_properties(rerun_algo PROPERTIES TIMEOUT 39)
set_tests_properties(rerun_content PROPERTIES TIMEOUT 20)
set_tests_properties(rivals_ui PROPERTIES TIMEOUT 80)
set_tests_properties(rerun_ui PROPERTIES TIMEOUT 60)
set_tests_properties(kast_growth PROPERTIES TIMEOUT 60)
set_tests_properties(kast_ui PROPERTIES TIMEOUT 60)
# Update 4 Phase A (SPEC v1.7 §6 PA-48, §8.1; the lead's P-4): the four new suites by the same rule. With empty mains their medians
# are about 0 s, so each TIMEOUT is its cap; re-measured at every gate's integration as their checks land.
set_tests_properties(festivals PROPERTIES TIMEOUT 34)
set_tests_properties(world_u4 PROPERTIES TIMEOUT 45)
set_tests_properties(big_field PROPERTIES TIMEOUT 30)
set_tests_properties(u4_ui PROPERTIES TIMEOUT 94)
# G1 (the lead's mid-wave commit H2; SPEC v1.7 §7.5, §8.1; PLAN_G1 §4.2 L8): the timing assertions' runs. A check that asserts
# milliseconds asserts them only behind SS_PERF=1, as the median of 5 runs, and prints them otherwise; these three run the same
# executables with SS_PERF=1, label perf and RUN_SERIAL (nothing else running beside them). The comparison tool's main run is
# "ctest -LE perf", which leaves them out; "ctest -L perf" runs them alone. festivals_perf: fest::All(), FestHour, the crown,
# RecordOf and HeroFestPins (§2.14); rival_sites_perf: "rivals <= 40% of RiverLine's build" (§8.5, tests/sites_main.cpp's
# RiverLineUnchanged); u4_ui_perf: the festival hubs' milliseconds (§2.8.10, §2.14's UI row, F-hubs' FestHubBudgets). A check
# whose code has not landed yet runs as its suite runs today and asserts once its track's package merges. Each TIMEOUT by P-4: its
# suite's cap is its spec budget (the medians add well under a second); re-measured with the suites at the gate's integration.
add_test(NAME festivals_perf COMMAND festivals_test)
add_test(NAME rival_sites_perf COMMAND sites_test)
add_test(NAME u4_ui_perf COMMAND u4_ui_test)
set_tests_properties(festivals_perf rival_sites_perf u4_ui_perf PROPERTIES ENVIRONMENT "SS_PERF=1" LABELS perf RUN_SERIAL TRUE)
set_tests_properties(festivals_perf PROPERTIES TIMEOUT 34)
set_tests_properties(rival_sites_perf PROPERTIES TIMEOUT 30)
set_tests_properties(u4_ui_perf PROPERTIES TIMEOUT 102)
# G1's integration (the lead's L-I.5; P-4, PLAN_G1 §4.2 L9, RULINGS R34-26 and "after the wave's fifth run" item 3): the suites G1
# changed and the three perf runs re-measured alone, three runs each (ctest -R, one at a time, 2026-11-13, the integrated tree), medians:
# festivals 15.45 s (31), rival_sites 12.38 s (30), u4_ui 46.90 s (94), synthesized_audio 5.46 s (11), monkey_play 67.07 s with its
# festival career (135), pro_badges 54.93 s (110), festivals_perf 15.41 s (31), rival_sites_perf 12.90 s (30), u4_ui_perf 50.71 s
# (102); each within its budget, so only the TIMEOUTs move. Every other suite keeps its Phase 0 TIMEOUT (SPEC v1.7 §8.1).

# Not a suite: core-styles' chart compiler (update 2 SPEC §2.4.2, §2.4.4), Tests/charts/*.txt into AI/ChartTables.cpp.
add_executable(chartgen tools/chartgen/chartgen_main.cpp)
target_link_libraries(chartgen PRIVATE ShortStackCore)
target_compile_definitions(chartgen PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")

# Not a suite: the tool that wrote Tests/fixtures/ from c09bc31's code (next_segment_spec §7.5) and update 2's day-0
# fixtures from bda07f3's (write2, update 2 SPEC §6.5), and the transcript that compares a build's sessions with that
# code's. It uses only what the session had then, so it builds against either.
add_executable(fixture_tool tests/fixture_tool.cpp)
target_link_libraries(fixture_tool PRIVATE ShortStackCore)

# Not a suite: the big field's calibration tool (update 4 SPEC v1.7 §4.1, §6.5 S-shadow, §6 PA-48; B-engine's from G2): it writes
# E/FieldModelCalib.inc and Tests/bigfield/dealt_<id>.txt. Registered in Phase A, doing nothing.
add_executable(bigfield_calib tests/bigfield_calib.cpp)
target_link_libraries(bigfield_calib PRIVATE ShortStackCore)
target_compile_definitions(bigfield_calib PRIVATE SS_TESTS_DIR="${SS_TESTS_DIR}")

# Not a suite either, and opt-in (-DSS_UNITY_PROBE=ON): the whole core pasted into one translation unit, in path order and
# in reverse, the way Unreal's unity blobs paste files together. A file's private names used through a namespace-scope
# using-directive leak into every file after it in a blob, so two such names that collide (TableHud.cpp's Margin and
# StoreCounter.cpp's, in Phase C) break the Unreal build only when adaptive unity happens to put the two files together.
# This compiles every arrangement at once. Compile only: nothing links or runs.
# Update 3 (update 3 SPEC §0.6 #4, §8.2): it also sees the shadowing Unreal's build treats as errors (C4456-C4459: a local
# hiding a local, a parameter, a member or a name another file's using-directive brought in, as 700d61f's C4459 in
# NetworkSites.cpp did), at warning level 1 with those four as errors; the rest stays quiet. /weNNNN alone both turns each on
# at /W1 and makes it an error: Visual Studio's generator puts /w1NNNN after /we on the command line, which would make them
# plain warnings again, so they aren't passed. Proven once by a canary (a deliberately shadowing translation unit failed the
# probe with C4456 and C4459 as errors, then went).
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
      target_compile_options(unity_probe_${Order} PRIVATE /W1 /we4456 /we4457 /we4458 /we4459 /bigobj /fp:precise /EHs-c- /GR- /permissive- /Zc:inline /Zc:preprocessor
        /Zc:strictStrings-)
      target_compile_definitions(unity_probe_${Order} PRIVATE _HAS_EXCEPTIONS=0 _CRT_SECURE_NO_WARNINGS)
    else()
      target_compile_options(unity_probe_${Order} PRIVATE -w -ffp-contract=off -fno-exceptions -fno-rtti)
    endif()
  endforeach()
endif()
