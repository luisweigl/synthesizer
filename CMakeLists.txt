cmake_minimum_required(VERSION 3.10)
project(synth VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)

add_executable(synth
    src/main.cpp
    src/waves.cpp
    src/voice.cpp
    src/envelope.cpp
    src/oscillator.cpp
    RtAudio.cpp
    RtMidi.cpp
)

target_include_directories(synth PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


target_compile_definitions(synth PRIVATE
    __UNIX_JACK__
)

target_link_libraries(synth PRIVATE
    PkgConfig::JACK
)