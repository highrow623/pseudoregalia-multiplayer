cmake_minimum_required(VERSION 3.18)

set(TARGET PseudoregaliaMultiplayerMod)
project(${TARGET})

add_library(${TARGET} SHARED "dllmain.cpp" "src/Client.cpp" "src/Logger.cpp")
target_include_directories(${TARGET} PRIVATE "include")
target_link_libraries(${TARGET} PUBLIC UE4SS)

target_compile_features(${TARGET} PUBLIC cxx_std_20)
