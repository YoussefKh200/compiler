#ifndef TIMER_H
#define TIMER_H

#include <chrono>

namespace compiler {

class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double, std::milli>;

    void start() {
        start_ = Clock::now();
        running_ = true;
    }

    void stop() {
        if (running_) {
            end_ = Clock::now();
            running_ = false;
        }
    }

    double elapsedMs() const {
        TimePoint end = running_ ? Clock::now() : end_;
        return Duration(end - start_).count();
    }

    double elapsedUs() const {
        return elapsedMs() * 1000.0;
    }

    void reset() {
        running_ = false;
    }

private:
    TimePoint start_;
    TimePoint end_;
    bool running_ = false;
};

} // namespace compiler

#endif // TIMER_H
