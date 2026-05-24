cmake_minimum_required(VERSION 3.16)
project(SEFERIM_AGI_G16 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(g16_v2_core INTERFACE)
target_include_directories(g16_v2_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(seferim STATIC
  src/seferim/seferim_federation.cpp
  src/seferim/allocator.cpp
)
target_include_directories(seferim PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(agi_bridge_v2 apps/agi_bridge_v2/main.cpp)
target_link_libraries(agi_bridge_v2 PRIVATE g16_v2_core seferim)

add_executable(agi_stream apps/agi_bridge_v2/main_streaming.cpp)
target_link_libraries(agi_stream PRIVATE g16_v2_core seferim)
find_package(Threads REQUIRED)
target_link_libraries(agi_stream PRIVATE Threads::Threads)

add_executable(test_g16_v2 tests/test_g16_v2.cpp)
target_link_libraries(test_g16_v2 PRIVATE g16_v2_core)

install(TARGETS agi_bridge_v2 RUNTIME DESTINATION bin)
