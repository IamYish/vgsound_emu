#include "sampler_host.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
constexpr uint32_t kFracBits = 11;
constexpr uint32_t kFracScale = 1u << kFracBits;

uint16_t velocity_to_volume(int velocity)
{
    const int clamped = std::clamp(velocity, 0, 127);
    const float normalized = static_cast<float>(clamped) / 127.0f;
    return static_cast<uint16_t>(normalized * 65535.0f);
}
}

SamplerHost::SamplerHost(uint32_t sample_rate, int root_note, float master_gain)
    : m_chip(*this)
    , m_sample_rate(sample_rate)
    , m_root_note(root_note)
    , m_master_gain(master_gain)
{
    m_chip.reset();
    m_chip.regs_w(0, 11, 31); // Activate all 32 voices.
}

void SamplerHost::load_sample(const LoadedSample &sample)
{
    m_sample_ram = sample.mono;
    if (m_sample_ram.size() < 2)
    {
        throw std::runtime_error("Sample must contain at least 2 frames");
    }
    const uint32_t end_index = static_cast<uint32_t>(m_sample_ram.size() - 1);
    m_sample_end_address = end_index << kFracBits;
}

void SamplerHost::note_on(int midi_note, int velocity)
{
    int chosen = -1;
    for (int i = 0; i < kVoiceCount; ++i)
    {
        if (!m_voices[i].active)
        {
            chosen = i;
            break;
        }
    }
    if (chosen < 0)
    {
        chosen = 0;
    }

    configure_voice(chosen, midi_note, velocity);
    m_voices[chosen].active = true;
    m_voices[chosen].midi_note = midi_note;
}

void SamplerHost::note_off(int midi_note)
{
    for (int i = 0; i < kVoiceCount; ++i)
    {
        if (m_voices[i].active && m_voices[i].midi_note == midi_note)
        {
            stop_voice(i);
            m_voices[i].active = false;
            m_voices[i].midi_note = -1;
        }
    }
}

void SamplerHost::render_block(int16_t *interleaved_stereo, size_t frames)
{
    for (size_t i = 0; i < frames; ++i)
    {
        // tick_perf() performs the low-cost ES5506 update path; one host call per output sample.
        m_chip.tick_perf();

        int32_t left_mix = 0;
        int32_t right_mix = 0;
        for (int ch = 0; ch < 6; ++ch)
        {
            left_mix += m_chip.lout(static_cast<u8>(ch));
            right_mix += m_chip.rout(static_cast<u8>(ch));
        }

        const float scaled_l = static_cast<float>(left_mix) * (m_master_gain / 32.0f);
        const float scaled_r = static_cast<float>(right_mix) * (m_master_gain / 32.0f);

        interleaved_stereo[i * 2 + 0] = static_cast<int16_t>(
          std::clamp(static_cast<int32_t>(scaled_l), static_cast<int32_t>(-32768), static_cast<int32_t>(32767)));
        interleaved_stereo[i * 2 + 1] = static_cast<int16_t>(
          std::clamp(static_cast<int32_t>(scaled_r), static_cast<int32_t>(-32768), static_cast<int32_t>(32767)));
    }
}

s16 SamplerHost::read_sample(u8, u8, u32 address)
{
    if (address >= m_sample_ram.size())
    {
        return 0;
    }
    return m_sample_ram[address];
}

void SamplerHost::configure_voice(int voice_index, int midi_note, int velocity)
{
    const double semitones = static_cast<double>(midi_note - m_root_note) / 12.0;
    const double ratio = std::pow(2.0, semitones);
    const uint32_t fc = static_cast<uint32_t>(std::clamp(ratio * static_cast<double>(kFracScale), 1.0, 131071.0));
    const uint16_t volume = velocity_to_volume(velocity);

    const u8 vp0 = static_cast<u8>(voice_index);
    const u8 vp1 = static_cast<u8>(voice_index + 32);

    // Voice page 32..63 registers.
    m_chip.regs_w(vp1, 1, 0);                    // START
    m_chip.regs_w(vp1, 2, m_sample_end_address); // END
    m_chip.regs_w(vp1, 3, 0);                    // ACCUM

    // Voice page 0..31 registers.
    m_chip.regs_w(vp0, 1, fc);     // FC
    m_chip.regs_w(vp0, 2, volume); // LVOL
    m_chip.regs_w(vp0, 4, volume); // RVOL
    m_chip.regs_w(vp0, 7, 0xffff); // K2 pass-through-ish
    m_chip.regs_w(vp0, 9, 0xffff); // K1 pass-through-ish
    m_chip.regs_w(vp0, 0, 0x0000); // CR: stop=0, channel=0
}

void SamplerHost::stop_voice(int voice_index)
{
    const u8 vp0 = static_cast<u8>(voice_index);
    m_chip.regs_w(vp0, 0, 0x0003); // stop bits set
}
