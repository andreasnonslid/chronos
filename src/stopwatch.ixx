module;
#include <chrono>
#include <vector>
export module stopwatch;

export struct Stopwatch {
    using tp = std::chrono::steady_clock::time_point;
    using dur = std::chrono::steady_clock::duration;

    void start(tp now) {
        if (!running_) {
            start_time_ = now;
            last_lap_time_ = now;
            running_ = true;
        }
    }

    void stop(tp now) {
        if (running_) {
            accumulated_ += now - start_time_;
            running_ = false;
        }
    }

    void lap(tp now) {
        if (running_) {
            laps_.push_back(now - last_lap_time_);
            last_lap_time_ = now;
            cumulative_ += laps_.back();
        }
    }

    void reset() {
        running_ = false;
        start_time_ = {};
        last_lap_time_ = {};
        accumulated_ = {};
        cumulative_ = {};
        laps_.clear();
    }

    dur elapsed(tp now) const {
        if (running_) return accumulated_ + (now - start_time_);
        return accumulated_;
    }

    bool is_running() const { return running_; }

    const std::vector<dur>& laps() const { return laps_; }

    dur cumulative() const { return cumulative_; }

  private:
    bool running_ = false;
    tp start_time_ = {};
    tp last_lap_time_ = {};
    dur accumulated_ = {};
    dur cumulative_ = {};
    std::vector<dur> laps_;
};
