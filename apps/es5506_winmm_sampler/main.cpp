#include "keyboard.h"
#include "sampler_host.h"
#include "wav_load.h"
#include "winmm_audio.h"

#include <Windows.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
struct Options
{
    std::string sample_path;
    int root_note = 60;
    float gain = 0.8f;
    uint32_t sample_rate = 48000;
};

Options parse_args(int argc, char **argv)
{
    Options opts;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--sample" && i + 1 < argc)
        {
            opts.sample_path = argv[++i];
        }
        else if (arg == "--root" && i + 1 < argc)
        {
            opts.root_note = std::stoi(argv[++i]);
        }
        else if (arg == "--gain" && i + 1 < argc)
        {
            opts.gain = std::stof(argv[++i]);
        }
        else if (arg == "--sample-rate" && i + 1 < argc)
        {
            opts.sample_rate = static_cast<uint32_t>(std::stoul(argv[++i]));
        }
        else
        {
            throw std::runtime_error("Unknown or malformed argument: " + arg);
        }
    }

    if (opts.sample_path.empty())
    {
        throw std::runtime_error("Usage: es5506_winmm_sampler --sample <path.wav> [--root 60] [--gain 0.8]");
    }

    opts.gain = std::clamp(opts.gain, 0.0f, 2.0f);
    return opts;
}

void print_help()
{
    std::cout << "ES5506 WinMM Sampler\n"
              << "ESC exits. Key map:\n"
              << "  Z X C V B N M , . /  -> 60..70\n"
              << "  A S D F G H J K L ;  -> 48..58\n"
              << "  Q W E R T Y U I O P  -> 72..81\n";
}
} // namespace

int main(int argc, char **argv)
{
    try
    {
        const Options opts = parse_args(argc, argv);

        LoadedSample loaded = load_wav_mono_16bit_pcm(opts.sample_path);
        loaded.mono = resample_linear(loaded.mono, loaded.sample_rate, opts.sample_rate);
        loaded.sample_rate = opts.sample_rate;

        SamplerHost sampler(opts.sample_rate, opts.root_note, opts.gain);
        sampler.load_sample(loaded);

        constexpr uint32_t kFramesPerBuffer = 512;
        constexpr uint32_t kBufferCount = 3;
        WinMmAudioEngine audio(opts.sample_rate,
                               kFramesPerBuffer,
                               kBufferCount,
                               [&sampler](int16_t *dst, size_t frames) { sampler.render_block(dst, frames); });
        audio.start();

        print_help();

        KeyboardInput keyboard;
        while ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) == 0)
        {
            const auto events = keyboard.poll_events();
            for (const auto &ev : events)
            {
                if (ev.pressed)
                {
                    sampler.note_on(ev.midi_note, 100);
                }
                else
                {
                    sampler.note_off(ev.midi_note);
                }
            }
            Sleep(3);
        }

        audio.stop();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
