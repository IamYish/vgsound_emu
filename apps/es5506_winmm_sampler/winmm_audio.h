#pragma once

#include <Windows.h>
#include <mmsystem.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

class WinMmAudioEngine
{
  public:
    using RenderFn = std::function<void(int16_t *dst, size_t frames)>;

    WinMmAudioEngine(uint32_t sample_rate, uint32_t frames_per_buffer, uint32_t buffer_count, RenderFn render_fn);
    ~WinMmAudioEngine();

    void start();
    void stop();

  private:
    static void CALLBACK wave_out_proc(HWAVEOUT hwo,
                                       UINT uMsg,
                                       DWORD_PTR dwInstance,
                                       DWORD_PTR dwParam1,
                                       DWORD_PTR dwParam2);
    void on_buffer_done(WAVEHDR *hdr);

    HWAVEOUT m_wave_out = nullptr;
    uint32_t m_frames_per_buffer = 0;
    RenderFn m_render_fn;
    std::vector<std::vector<int16_t>> m_audio_blocks;
    std::vector<WAVEHDR> m_headers;
    std::atomic<bool> m_running{false};
};
