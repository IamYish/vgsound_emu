# ES5506 WinMM Sampler (Windows)

Minimal standalone ES5506 keyboard sampler using WinMM `waveOut*` and `GetAsyncKeyState`.

## Build (Windows 10/11 x64)

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target es5506_winmm_sampler
```

Executable output is in `build/apps/es5506_winmm_sampler/Release/` (or your generator's equivalent path).

## Run

```powershell
es5506_winmm_sampler --sample "C:\path\sample.wav" [--root 60] [--gain 0.8] [--sample-rate 48000]
```

Supported WAV input for v0:
- 16-bit PCM
- mono or stereo (stereo uses left channel)

Keyboard map:
- `Z X C V B N M , . /` -> MIDI `60..70`
- `A S D F G H J K L ;` -> MIDI `48..58`
- `Q W E R T Y U I O P` -> MIDI `72..81`

Press `ESC` to exit cleanly.

## Architecture note

- **Audio buffering / latency:** uses **3 buffers** (`WAVEHDR`) of **512 frames** each at 48 kHz by default. This is about 10.67 ms per buffer and ~32 ms total queued audio.
- **Key edge detection:** each mapped key tracks its previous pressed state; `GetAsyncKeyState` is polled every ~3 ms. Transition `up->down` emits `NoteOn`, transition `down->up` emits `NoteOff`.
- **Pitch increment:** ES5506 accumulator uses 11 fractional bits. Frequency control is `fc = 2^((note-root)/12) * 2^11`, clamped to the 17-bit FC register.
