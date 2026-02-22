cmake_minimum_required(VERSION 3.16)
project(vgsound_emu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VGSOUND_ES550X_SOURCES
    src/es550x/es550x.cpp
    src/es550x/es550x_alu.cpp
    src/es550x/es550x_filter.cpp
    src/es550x/es5506.cpp
)

add_library(vgsound_es550x STATIC ${VGSOUND_ES550X_SOURCES})
target_include_directories(vgsound_es550x PUBLIC src)

if(WIN32)
    add_subdirectory(apps/es5506_winmm_sampler)
endif()
