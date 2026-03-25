module;
#include <chrono>
export module timer;

export struct Timer {
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

  private:
    bool running_ = false;
    bool touched_ = false;
    tp start_time_ = {};
    dur target_ = {};
    dur elapsed_at_pause_ = {};
};
