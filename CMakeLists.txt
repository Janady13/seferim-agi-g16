cmake_minimum_required(VERSION 3.16)
project(SEFERIM_AGI_G16 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(g16_v2_core INTERFACE)
target_include_directories(g16_v2_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(g16_v2_core INTERFACE cxx_std_20)

add_library(g16_core STATIC
  src/common/g16_core.cpp
)
target_include_directories(g16_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(g16_core PUBLIC g16_v2_core)

add_library(seferim STATIC
  src/seferim/seferim_federation.cpp
  src/seferim/allocator.cpp
)
target_include_directories(seferim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(seferim PUBLIC g16_v2_core)

add_executable(agi_bridge_v2 apps/agi_bridge_v2/main.cpp)
target_link_libraries(agi_bridge_v2 PRIVATE g16_v2_core seferim)

add_executable(agi_stream apps/agi_bridge_v2/main_streaming.cpp)
target_link_libraries(agi_stream PRIVATE g16_v2_core seferim)
find_package(Threads REQUIRED)
target_link_libraries(agi_stream PRIVATE Threads::Threads)

add_executable(test_g16_v2 tests/test_g16_v2.cpp)
target_link_libraries(test_g16_v2 PRIVATE g16_v2_core)

add_executable(agi_core apps/agi_core/main.cpp)
target_link_libraries(agi_core PRIVATE g16_core)

add_executable(agi_loop apps/agi_loop/main.cpp)
target_link_libraries(agi_loop PRIVATE g16_core)

add_executable(agi_opt apps/agi_opt/main.cpp)
target_link_libraries(agi_opt PRIVATE g16_core)

add_executable(agi_profile apps/agi_profile/main.cpp)
target_link_libraries(agi_profile PRIVATE g16_core)

add_executable(agi_step apps/agi_step/main.cpp)
target_link_libraries(agi_step PRIVATE g16_core)

add_executable(g16_core_tests tests/g16_core_tests.cpp)
target_link_libraries(g16_core_tests PRIVATE g16_core)

enable_testing()
add_test(NAME test_g16_v2 COMMAND test_g16_v2)
add_test(NAME g16_core_tests COMMAND g16_core_tests)

install(TARGETS agi_bridge_v2 RUNTIME DESTINATION bin)
