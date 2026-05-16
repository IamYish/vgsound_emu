#include "Es5506OnlyEmu.h"
#include "WavWriter.h"

#include <cmath>
#include <filesystem>
#include <iostream>

int main() {
    constexpr uint32_t kRenderTicks = 44100;
    constexpr uint32_t kSampleRate = 44100;
    constexpr size_t kSamplePoints = 4096;

    Es5506OnlyEmu emu;
    emu.reset();

    std::vector<int16_t> sample_points;
    sample_points.reserve(kSamplePoints);
    for (size_t i = 0; i < kSamplePoints; ++i) {
        const double phase = (2.0 * M_PI * static_cast<double>(i)) / 256.0;
        sample_points.push_back(static_cast<int16_t>(std::sin(phase) * 22000.0));
    }
    emu.setSamplePoints(sample_points);

    emu.configureOneVoice(0, static_cast<uint32_t>(kSamplePoints - 1), 0, 0x0400, 0x7f00, 0x7f00, 0x0308);

    auto pcm = emu.renderTicksToStereo16(kRenderTicks);

    std::filesystem::create_directories("renders");
    const bool wav_ok = writeStereo16Wav("renders/es5506_one_voice.wav", pcm, kSampleRate);

    const auto& c = emu.counters();
    std::cout << "total ticks: " << c.total_ticks << "\n";
    std::cout << "sample reads: " << c.sample_reads << "\n";
    std::cout << "voice update count: " << c.voice_updates << "\n";
    std::cout << "voice wheel wrap count: " << c.voice_wheel_wraps << "\n";
    std::cout << "nonzero output count: " << c.nonzero_outputs << "\n";
    std::cout << "first nonzero tick: " << c.first_nonzero_tick << "\n";

    const bool pass = wav_ok && (c.sample_reads > 0) && (c.voice_updates > 0) && (c.nonzero_outputs > 0);
    std::cout << (pass ? "SELF_CHECK: PASS" : "SELF_CHECK: FAIL") << "\n";
    return pass ? 0 : 1;
}
