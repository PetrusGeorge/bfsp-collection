#ifndef RNG_H
#define RNG_H

#include <random>
class RNG {
  public:
    // Singleton pattern
    RNG(const RNG &) = delete;
    RNG &operator=(const RNG &) = delete;
    RNG(RNG &&) = delete;
    RNG &operator=(RNG &&) = delete;
    ~RNG() = default;

    static RNG &instance() {
        static RNG instance;
        return instance;
    }

    const std::mt19937 &gen() { return m_gen; }
    void set_seed(unsigned int seed) { m_gen.seed(seed); }

    // If any more functions are needed make them here on this class
    template <typename T> T generate(T min, T max) {
        static_assert(std::is_integral_v<T>, "must be an integer type");
        std::uniform_int_distribution<T> dis(min, max);
        return dis(m_gen);
    }

  private:
    RNG() {}

    // Random seed if no seed is setted
    std::mt19937 m_gen{std::random_device{}()};
};

#endif
