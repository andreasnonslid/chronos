#pragma once
#include <chrono>

struct Timer {
    using tp = std::chrono::steady_clock::time_point;
    using dur = std::chrono::steady_clock::duration;

    void set(dur d) { target_ = d; }

    void start(tp now) {
        if (!running_) {
            start_time_ = now;
            running_ = true;
            touched_ = true;
        }
    }

    void pause(tp now) {
        if (running_) {
            elapsed_at_pause_ += now - start_time_;
            running_ = false;
        }
    }

    void reset() {
        running_ = false;
        touched_ = false;
        start_time_ = {};
        target_ = {};
        elapsed_at_pause_ = {};
    }

    dur remaining(tp now) const {
        dur elapsed = elapsed_at_pause_;
        if (running_) elapsed += now - start_time_;
        if (elapsed >= target_) return dur::zero();
        return target_ - elapsed;
    }

    bool expired(tp now) const { return target_ > dur::zero() && remaining(now) == dur::zero(); }

    bool is_running() const { return running_; }
    bool touched() const { return touched_; }

    dur total_elapsed(tp now) const {
        dur e = elapsed_at_pause_;
        if (running_) e += now - start_time_;
        return e;
    }

    void restore(dur target, dur elapsed_paused, bool running, tp now) {
        reset();
        target_ = target;
        elapsed_at_pause_ = elapsed_paused;
        touched_ = elapsed_paused > dur::zero() || running;
        if (running) {
            start_time_ = now;
            running_ = true;
        }
    }

  private:
    bool running_ = false;
    bool touched_ = false;
    tp start_time_ = {};
    dur target_ = {};
    dur elapsed_at_pause_ = {};
};
