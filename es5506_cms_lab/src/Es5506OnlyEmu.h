#pragma once

#include "Es5506Memory.h"
#include "../third_party/vgsound_emu/src/es550x/es5506.hpp"
#include <cstdint>
#include <vector>

struct Es5506TraceCounters {
    uint64_t total_ticks = 0;
    uint64_t voice_updates = 0;
    uint64_t voice_wheel_wraps = 0;
    uint64_t sample_reads = 0;
    uint64_t nonzero_outputs = 0;
    int64_t first_nonzero_tick = -1;
};

class Es5506OnlyEmu {
public:
    Es5506OnlyEmu();
    void reset();
    void tick(uint32_t count);
    void configureOneVoice(uint32_t start, uint32_t end, uint32_t accum, uint32_t fc, uint16_t lvol, uint16_t rvol, uint16_t cr);
    std::vector<int16_t> renderTicksToStereo16(uint32_t count);
    void setSamplePoints(std::vector<int16_t> points);
    const Es5506TraceCounters& counters() const { return counters_; }

private:
    struct Intf : public es550x_intf {
        explicit Intf(Es5506Memory& mem) : mem(mem) {}
        s16 read_sample(u8 voice, u8 bank, u32 address) override { return mem.read_sample(voice, bank, address); }
        Es5506Memory& mem;
    };

    static int16_t clamp16(int32_t v);

    Es5506Memory memory_;
    Intf intf_;
    es5506_core chip_;
    Es5506TraceCounters counters_;
};
