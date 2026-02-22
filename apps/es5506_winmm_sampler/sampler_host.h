#pragma once

#include "wav_load.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "es550x/es5506.hpp"

class SamplerHost : public es550x_intf
{
  public:
    SamplerHost(uint32_t sample_rate, int root_note, float master_gain);

    void load_sample(const LoadedSample &sample);
    void note_on(int midi_note, int velocity);
    void note_off(int midi_note);
    void render_block(int16_t *interleaved_stereo, size_t frames);

    virtual s16 read_sample(u8 voice, u8 bank, u32 address) override;

  private:
    struct VoiceState
    {
        bool active = false;
        int midi_note = -1;
    };

    void configure_voice(int voice_index, int midi_note, int velocity);
    void stop_voice(int voice_index);

    static constexpr int kVoiceCount = 32;
    es5506_core m_chip;
    uint32_t m_sample_rate = 48000;
    int m_root_note = 60;
    float m_master_gain = 0.8f;

    std::vector<int16_t> m_sample_ram;
    uint32_t m_sample_end_address = 0;
    std::array<VoiceState, kVoiceCount> m_voices{};
};
