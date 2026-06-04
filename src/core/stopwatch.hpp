#pragma once
#include <chrono>
#include <vector>
#include "assert.hpp"

struct Stopwatch {
    using tp = std::chrono::steady_clock::time_point;
    using dur = std::chrono::steady_clock::duration;

    void start(tp now) {
        CHRONOS_ASSERT(!running_);
        start_time_ = now;
        running_ = true;
    }

    void stop(tp now) {
        CHRONOS_ASSERT(running_);
        accumulated_ += now - start_time_;
        running_ = false;
    }

    static constexpr size_t MAX_LAPS = 999;

    void lap(tp now) {
        CHRONOS_ASSERT(running_);
        if (laps_.size() < MAX_LAPS) {
            const dur current = elapsed(now);
            laps_.push_back(current - last_lap_elapsed_);
            last_lap_elapsed_ = current;
        }
    }

    void reset() {
        running_ = false;
        start_time_ = {};
        last_lap_elapsed_ = {};
        accumulated_ = {};
        laps_.clear();
    }

    bool is_running() const { return running_; }
    bool touched() const { return running_ || accumulated_.count() != 0 || !laps_.empty(); }

    dur elapsed(tp now) const {
        return accumulated_ + (running_ ? now - start_time_ : dur{});
    }

    dur total_elapsed(tp now) const { return elapsed(now); }

    const std::vector<dur>& laps() const { return laps_; }

    dur cumulative() const { return last_lap_elapsed_; }

private:
    bool running_ = false;
    tp   start_time_{};
    dur  accumulated_{};
    dur  last_lap_elapsed_{};
    std::vector<dur> laps_;
};
