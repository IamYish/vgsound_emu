#include "Es5506OnlyEmu.h"

Es5506OnlyEmu::Es5506OnlyEmu() : intf_(memory_), chip_(intf_) {}

void Es5506OnlyEmu::reset() {
    chip_.reset();
    memory_.reset_counters();
    counters_ = {};
}

void Es5506OnlyEmu::setSamplePoints(std::vector<int16_t> points) { memory_.setSamplePoints(std::move(points)); }

void Es5506OnlyEmu::configureOneVoice(uint32_t start, uint32_t end, uint32_t accum, uint32_t fc, uint16_t lvol, uint16_t rvol, uint16_t cr) {
    chip_.regs_w(0x20, 0x00, cr);
    chip_.regs_w(0x20, 0x01, start << 11);
    chip_.regs_w(0x20, 0x02, end << 11);
    chip_.regs_w(0x20, 0x03, accum << 11);
    chip_.regs_w(0x00, 0x01, fc);
    chip_.regs_w(0x00, 0x02, lvol);
    chip_.regs_w(0x00, 0x04, rvol);
    chip_.regs_w(0x00, 0x0b, 0); // ACT: minimum voice window (chip clamps to 5 voices)
    chip_.regs_w(0x00, 0x0c, 0x08); // MODE master, serial clocks disabled
    chip_.regs_w(0x20, 0x0a, 0); // W_ST
    chip_.regs_w(0x20, 0x0b, 20); // W_END for 20-bit serial output
    chip_.regs_w(0x20, 0x0c, 40); // LR_END
}

void Es5506OnlyEmu::tick(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        chip_.tick();
        ++counters_.total_ticks;
        if (chip_.voice_update()) ++counters_.voice_updates;
        if (chip_.voice_end()) ++counters_.voice_wheel_wraps;
        const int32_t l = chip_.voice_lout(0);
        const int32_t r = chip_.voice_rout(0);
        if (l != 0 || r != 0) {
            ++counters_.nonzero_outputs;
            if (counters_.first_nonzero_tick < 0) counters_.first_nonzero_tick = static_cast<int64_t>(counters_.total_ticks - 1);
        }
    }
    counters_.sample_reads = memory_.sample_reads();
}

int16_t Es5506OnlyEmu::clamp16(int32_t v) {
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return static_cast<int16_t>(v);
}

std::vector<int16_t> Es5506OnlyEmu::renderTicksToStereo16(uint32_t count) {
    std::vector<int16_t> out;
    out.reserve(static_cast<size_t>(count) * 2);
    for (uint32_t i = 0; i < count; ++i) {
        chip_.tick();
        ++counters_.total_ticks;
        if (chip_.voice_update()) ++counters_.voice_updates;
        if (chip_.voice_end()) ++counters_.voice_wheel_wraps;
        int32_t l = chip_.voice_lout(0) >> 4;
        int32_t r = chip_.voice_rout(0) >> 4;
        out.push_back(clamp16(l));
        out.push_back(clamp16(r));
        if (l != 0 || r != 0) {
            ++counters_.nonzero_outputs;
            if (counters_.first_nonzero_tick < 0) counters_.first_nonzero_tick = static_cast<int64_t>(counters_.total_ticks - 1);
        }
    }
    counters_.sample_reads = memory_.sample_reads();
    return out;
}
