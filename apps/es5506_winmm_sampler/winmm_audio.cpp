#include "winmm_audio.h"

#include <stdexcept>

WinMmAudioEngine::WinMmAudioEngine(uint32_t sample_rate,
                                   uint32_t frames_per_buffer,
                                   uint32_t buffer_count,
                                   RenderFn render_fn)
    : m_frames_per_buffer(frames_per_buffer)
    , m_render_fn(std::move(render_fn))
    , m_audio_blocks(buffer_count)
    , m_headers(buffer_count)
{
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = sample_rate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>((format.nChannels * format.wBitsPerSample) / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    MMRESULT open_res = waveOutOpen(&m_wave_out,
                                    WAVE_MAPPER,
                                    &format,
                                    reinterpret_cast<DWORD_PTR>(&WinMmAudioEngine::wave_out_proc),
                                    reinterpret_cast<DWORD_PTR>(this),
                                    CALLBACK_FUNCTION);
    if (open_res != MMSYSERR_NOERROR)
    {
        throw std::runtime_error("waveOutOpen failed");
    }

    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        m_audio_blocks[i].resize(frames_per_buffer * 2);
        WAVEHDR &hdr = m_headers[i];
        hdr.lpData = reinterpret_cast<LPSTR>(m_audio_blocks[i].data());
        hdr.dwBufferLength = static_cast<DWORD>(m_audio_blocks[i].size() * sizeof(int16_t));
        hdr.dwFlags = 0;
    }
}

WinMmAudioEngine::~WinMmAudioEngine() { stop(); }

void WinMmAudioEngine::start()
{
    if (m_running.exchange(true))
    {
        return;
    }

    for (size_t i = 0; i < m_headers.size(); ++i)
    {
        m_render_fn(m_audio_blocks[i].data(), m_frames_per_buffer);
        MMRESULT prep = waveOutPrepareHeader(m_wave_out, &m_headers[i], sizeof(WAVEHDR));
        if (prep != MMSYSERR_NOERROR)
        {
            throw std::runtime_error("waveOutPrepareHeader failed");
        }
        MMRESULT wr = waveOutWrite(m_wave_out, &m_headers[i], sizeof(WAVEHDR));
        if (wr != MMSYSERR_NOERROR)
        {
            throw std::runtime_error("waveOutWrite failed");
        }
    }
}

void WinMmAudioEngine::stop()
{
    if (!m_wave_out)
    {
        return;
    }

    m_running = false;
    waveOutReset(m_wave_out);

    for (auto &hdr : m_headers)
    {
        if (hdr.dwFlags & WHDR_PREPARED)
        {
            waveOutUnprepareHeader(m_wave_out, &hdr, sizeof(WAVEHDR));
        }
    }

    waveOutClose(m_wave_out);
    m_wave_out = nullptr;
}

void CALLBACK WinMmAudioEngine::wave_out_proc(HWAVEOUT,
                                              UINT uMsg,
                                              DWORD_PTR dwInstance,
                                              DWORD_PTR dwParam1,
                                              DWORD_PTR)
{
    if (uMsg != WOM_DONE)
    {
        return;
    }

    auto *engine = reinterpret_cast<WinMmAudioEngine *>(dwInstance);
    auto *hdr = reinterpret_cast<WAVEHDR *>(dwParam1);
    engine->on_buffer_done(hdr);
}

void WinMmAudioEngine::on_buffer_done(WAVEHDR *hdr)
{
    if (!m_running)
    {
        return;
    }

    m_render_fn(reinterpret_cast<int16_t *>(hdr->lpData), m_frames_per_buffer);
    waveOutWrite(m_wave_out, hdr, sizeof(WAVEHDR));
}
