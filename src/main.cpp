#include "NaiveQueue.hpp"
#include "SPSCQueue.hpp"
#include "SizeQueue.hpp"
#include "cpuset.hpp"
#include "queue.hpp"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <format>
#include <latch>
#include <thread>
#include <vector>

struct rand_gen {
  uint32_t seed;
  auto rnd() {
    seed = (214013 * seed + 2531011);
    return (seed >> 16) & 0x7FFF;
  }
};

auto test_once(SPSCQueue<size_t> auto q, size_t num_count) {
  std::latch thread_init(2);
  std::latch thread_start(1);
  std::latch thread_done(2);
  // rand_gen gen1{.seed = 0x12345678};
  // rand_gen gen2{.seed = 0x12345679};
  std::jthread reader([&] {
    cpuset cpuset1;
    cpuset1.insert(1);
    set_cpu_affinity(cpuset1);
    thread_init.count_down();
    thread_start.wait();
    size_t received = 0;
    auto reader = q.getReader();
    while (received < num_count) {
      // if ((gen1.rnd() % 3) == 0)
      // continue;
      auto res = reader->pop();
      assert(res == received);
      ++received;
    }
    thread_done.count_down();
  });
  std::jthread writer([&] {
    cpuset cpuset2;
    cpuset2.insert(2);
    set_cpu_affinity(cpuset2);

    thread_init.count_down();
    thread_start.wait();
    auto writer = q.getWriter();
    for (size_t cur = 0; cur < num_count; ++cur) {
      // while ((gen2.rnd() % 3) != 0) {}
      writer->push(cur);
    }
    thread_done.count_down();
  });
  thread_init.wait();
  auto start_time = std::chrono::high_resolution_clock::now();
  thread_start.count_down();
  thread_done.wait();
  auto end_time = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                              start_time)
      .count();
}

auto test_times(const auto &queue_generator, size_t num_count,
                size_t num_times) {
  std::vector<double> times;
  for (size_t i = 0; i < num_times; ++i) {
    times.emplace_back(1.0 * test_once(queue_generator(), num_count) /
                       num_count);
    printf("test %zu done\n", i);
  }
  return times;
}

auto analyze_times(const auto &times) {
  double avg = 0;
  double min = std::numeric_limits<double>::max();
  double max = 0;
  for (auto time : times) {
    avg += 1.0 * time / times.size();
    min = std::min(min, time);
    max = std::max(max, time);
  }
  return std::make_tuple(avg, min, max);
}

int main() {
  // auto test_result = test_times([] { return NaiveQueue<size_t>{}; }, 1e6,50);
  // auto test_result = test_times([] { return SizeQueue<size_t>{}; }, 1e6, 50);

  auto test_result = test_times([] { return LightQueue<size_t>{}; }, 2e6, 200);
  auto res = analyze_times(test_result);
  puts(std::format("NaiveQueue: avg: {} ns, min: {} ns, max: {} ns\n",
                   std::get<0>(res), std::get<1>(res), std::get<2>(res))
           .c_str());
  return 0;
}
