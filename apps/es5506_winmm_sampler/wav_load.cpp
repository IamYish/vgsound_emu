#include "wav_load.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace
{
template <typename T> T read_le(const uint8_t *p)
{
    T value = 0;
    std::memcpy(&value, p, sizeof(T));
    return value;
}
}

LoadedSample load_wav_mono_16bit_pcm(const std::string &path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Failed to open WAV file: " + path);
    }

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (bytes.size() < 44)
    {
        throw std::runtime_error("WAV file too small: " + path);
    }

    if (std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
    {
        throw std::runtime_error("Not a RIFF/WAVE file: " + path);
    }

    size_t offset = 12;
    uint16_t audio_format = 0;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    uint16_t bits_per_sample = 0;
    const uint8_t *data_ptr = nullptr;
    uint32_t data_size = 0;

    while (offset + 8 <= bytes.size())
    {
        const uint8_t *chunk = bytes.data() + offset;
        const uint32_t chunk_size = read_le<uint32_t>(chunk + 4);
        const size_t chunk_data_offset = offset + 8;
        if (chunk_data_offset + chunk_size > bytes.size())
        {
            throw std::runtime_error("Malformed WAV chunk in: " + path);
        }

        if (std::memcmp(chunk, "fmt ", 4) == 0)
        {
            if (chunk_size < 16)
            {
                throw std::runtime_error("Invalid fmt chunk in: " + path);
            }
            audio_format = read_le<uint16_t>(bytes.data() + chunk_data_offset + 0);
            channels = read_le<uint16_t>(bytes.data() + chunk_data_offset + 2);
            sample_rate = read_le<uint32_t>(bytes.data() + chunk_data_offset + 4);
            bits_per_sample = read_le<uint16_t>(bytes.data() + chunk_data_offset + 14);
        }
        else if (std::memcmp(chunk, "data", 4) == 0)
        {
            data_ptr = bytes.data() + chunk_data_offset;
            data_size = chunk_size;
        }

        offset = chunk_data_offset + chunk_size + (chunk_size & 1U);
    }

    if (audio_format != 1 || (channels != 1 && channels != 2) || bits_per_sample != 16 || !data_ptr)
    {
        throw std::runtime_error("Only 16-bit PCM mono/stereo WAV is supported for v0");
    }

    const size_t frame_count = data_size / (channels * sizeof(int16_t));
    LoadedSample out;
    out.sample_rate = sample_rate;
    out.mono.resize(frame_count);

    const int16_t *samples = reinterpret_cast<const int16_t *>(data_ptr);
    if (channels == 1)
    {
        std::copy(samples, samples + frame_count, out.mono.begin());
    }
    else
    {
        for (size_t i = 0; i < frame_count; ++i)
        {
            out.mono[i] = samples[i * 2];
        }
    }

    return out;
}

std::vector<int16_t> resample_linear(const std::vector<int16_t> &in, uint32_t in_rate, uint32_t out_rate)
{
    if (in.empty() || in_rate == 0 || out_rate == 0)
    {
        return {};
    }
    if (in_rate == out_rate)
    {
        return in;
    }

    const double ratio = static_cast<double>(in_rate) / static_cast<double>(out_rate);
    const size_t out_count = static_cast<size_t>((static_cast<double>(in.size()) / ratio));

    std::vector<int16_t> out(out_count);
    for (size_t i = 0; i < out_count; ++i)
    {
        const double src_pos = static_cast<double>(i) * ratio;
        const size_t idx = static_cast<size_t>(src_pos);
        const double frac = src_pos - static_cast<double>(idx);

        const int16_t s0 = in[std::min(idx, in.size() - 1)];
        const int16_t s1 = in[std::min(idx + 1, in.size() - 1)];
        const double mixed = static_cast<double>(s0) + (static_cast<double>(s1) - static_cast<double>(s0)) * frac;
        out[i] = static_cast<int16_t>(std::clamp(mixed, -32768.0, 32767.0));
    }

    return out;
}
