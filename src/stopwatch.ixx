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
            dur current = elapsed(now);
            laps_.push_back(current - last_lap_elapsed_);
            last_lap_elapsed_ = current;
            cumulative_ += laps_.back();
        }
    }

    void reset() {
        running_ = false;
        start_time_ = {};
        last_lap_elapsed_ = {};
        accumulated_ = {};
        cumulative_ = {};
        laps_.clear();
    }

    dur elapsed(tp now) const {
        if (running_) return accumulated_ + (now - start_time_);
        return accumulated_;
    }

    bool is_running() const { return running_; }

    void restore(dur accumulated, bool running, tp now) {
        reset();
        accumulated_ = accumulated;
        last_lap_elapsed_ = accumulated;
        if (running) {
            start_time_ = now;
            running_ = true;
        }
    }

    const std::vector<dur>& laps() const { return laps_; }

    dur cumulative() const { return cumulative_; }

  private:
    bool running_ = false;
    tp start_time_ = {};
    dur last_lap_elapsed_ = {};
    dur accumulated_ = {};
    dur cumulative_ = {};
    std::vector<dur> laps_;
};
