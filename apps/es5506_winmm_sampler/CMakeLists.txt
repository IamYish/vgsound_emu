add_executable(es5506_winmm_sampler
    main.cpp
    winmm_audio.cpp
    wav_load.cpp
    keyboard.cpp
    sampler_host.cpp
)

target_include_directories(es5506_winmm_sampler PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(es5506_winmm_sampler PRIVATE vgsound_es550x winmm)

if(MSVC)
    target_compile_options(es5506_winmm_sampler PRIVATE /W4 /permissive-)
else()
    target_compile_options(es5506_winmm_sampler PRIVATE -Wall -Wextra -Wpedantic)
endif()
