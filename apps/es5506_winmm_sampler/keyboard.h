#pragma once

#include <vector>

struct KeyEvent
{
    int midi_note = 0;
    bool pressed = false;
};

class KeyboardInput
{
  public:
    KeyboardInput();
    std::vector<KeyEvent> poll_events();

  private:
    struct Binding
    {
        int virtual_key = 0;
        int midi_note = 0;
        bool previous_down = false;
    };

    std::vector<Binding> m_bindings;
};
