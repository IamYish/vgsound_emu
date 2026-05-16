#pragma once

#include <cstdint>
#include <vector>

class Es5506Memory {
public:
    void setSamplePoints(std::vector<int16_t> points) { sample_points_ = std::move(points); }

    int16_t read_sample(uint8_t /*voice*/, uint8_t /*bank*/, uint32_t address) {
        ++sample_reads_;
        if (address >= sample_points_.size()) {
            return 0;
        }
        return sample_points_[address];
    }

    uint64_t sample_reads() const { return sample_reads_; }
    void reset_counters() { sample_reads_ = 0; }

private:
    std::vector<int16_t> sample_points_;
    uint64_t sample_reads_ = 0;
};
