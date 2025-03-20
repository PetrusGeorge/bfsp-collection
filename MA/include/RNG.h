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

  std::mt19937 &gen() { return m_gen; }
  size_t seed() const { return m_seed; }
  void set_seed(size_t seed) { 
    m_seed = seed; 
    m_gen.seed(seed);
  }

  // If any more functions are needed make them here on this class
  template <typename T> T generate(T min, T max) {
    static_assert(std::is_integral_v<T>, "must be an integer type");
    std::uniform_int_distribution<T> dis(min, max);
    return dis(m_gen);
  }

  double generateDouble() {
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(m_gen);
  }


private:
  RNG() = default;

  // Random seed if no seed is set
  size_t m_seed = std::random_device{}();
  std::mt19937 m_gen{m_seed};
};

#endif