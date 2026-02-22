#include "keyboard.h"

#include <Windows.h>

namespace
{
struct RowBinding
{
    int virtual_key;
    int midi_note;
};
}

KeyboardInput::KeyboardInput()
{
    const std::vector<RowBinding> rows = {
      {'Z', 60},      {'X', 61},      {'C', 62},      {'V', 63},      {'B', 64},
      {'N', 65},      {'M', 66},      {VK_OEM_COMMA, 67}, {VK_OEM_PERIOD, 68}, {VK_OEM_2, 69},
      {'A', 48},      {'S', 49},      {'D', 50},      {'F', 51},      {'G', 52},
      {'H', 53},      {'J', 54},      {'K', 55},      {'L', 56},      {VK_OEM_1, 57},
      {'Q', 72},      {'W', 73},      {'E', 74},      {'R', 75},      {'T', 76},
      {'Y', 77},      {'U', 78},      {'I', 79},      {'O', 80},      {'P', 81},
    };

    for (const auto &entry : rows)
    {
        m_bindings.push_back({entry.virtual_key, entry.midi_note, false});
    }
}

std::vector<KeyEvent> KeyboardInput::poll_events()
{
    std::vector<KeyEvent> events;
    for (auto &binding : m_bindings)
    {
        const bool down = (GetAsyncKeyState(binding.virtual_key) & 0x8000) != 0;
        if (down != binding.previous_down)
        {
            events.push_back({binding.midi_note, down});
            binding.previous_down = down;
        }
    }
    return events;
}
