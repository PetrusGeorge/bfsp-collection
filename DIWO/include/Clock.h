#ifndef CLOCK_H
#define CLOCK_H

#include <chrono>

class Clock {
  public:
    std::chrono::time_point<std::chrono::steady_clock> beginning;
    std::chrono::time_point<std::chrono::steady_clock> end;
    void start() { beginning = std::chrono::steady_clock::now(); }
    void stop() { end = std::chrono::steady_clock::now(); }
    template <typename T> [[nodiscard]] size_t get_elapsed() const {
        return std::chrono::duration_cast<T>(end - beginning).count();
    }
    template <typename T> [[nodiscard]] size_t get_elapsed_now() const {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<T>(now - beginning).count();
    }
};

#endif
