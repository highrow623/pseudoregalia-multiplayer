cmake_minimum_required(VERSION 3.18)

set(TARGET PseudoregaliaMultiplayerMod)
project(${TARGET})

add_library(${TARGET} SHARED "dllmain.cpp" "src/Client.cpp" "src/Logger.cpp")
target_include_directories(${TARGET} PRIVATE "include")
target_include_directories(${TARGET} PRIVATE "deps/wswrap/include")
target_include_directories(${TARGET} PRIVATE "deps/asio/include")
target_include_directories(${TARGET} PRIVATE "deps/websocketpp")
target_include_directories(${TARGET} PRIVATE "deps/json/include")
target_link_libraries(${TARGET} PUBLIC UE4SS)

target_compile_features(${TARGET} PUBLIC cxx_std_20)

target_compile_definitions(${TARGET} PRIVATE _WIN32_WINNT=0x0600)

target_compile_options(${TARGET} PRIVATE /Zc:__cplusplus)
