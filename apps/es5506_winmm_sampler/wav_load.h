#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct LoadedSample
{
    std::vector<int16_t> mono;
    uint32_t sample_rate = 0;
};

LoadedSample load_wav_mono_16bit_pcm(const std::string &path);
std::vector<int16_t> resample_linear(const std::vector<int16_t> &in,
                                     uint32_t in_rate,
                                     uint32_t out_rate);
