cmake_minimum_required(VERSION 3.16)
project(es5506_cms_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(es5506_cms_lab
    src/main.cpp
    src/Es5506OnlyEmu.cpp
    third_party/vgsound_emu/src/es550x/es550x.cpp
    third_party/vgsound_emu/src/es550x/es550x_filter.cpp
    third_party/vgsound_emu/src/es550x/es550x_alu.cpp
    third_party/vgsound_emu/src/es550x/es5506.cpp
)

target_include_directories(es5506_cms_lab PRIVATE
    src
    third_party/vgsound_emu/src
)
