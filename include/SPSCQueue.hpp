#pragma once

#include "queue.hpp"
#include <array>
#include <atomic>
#include <memory>

template <class U, size_t capacity = 1024> class LightQueue {
  class Reader {
  public:
    Reader(LightQueue<U> &p) : p(p) { p.has_reader = true; }
    Reader(Reader const &) = delete;
    Reader(Reader &&) = default;
    ~Reader() { p.has_reader = false; }
    auto front() {
      return p.q.at(p.read_idx.load(std::memory_order_acquire) % capacity)
          .inner;
    }
    auto pop() noexcept { return p.pop(); }
    auto isEmpty() const { return p.isEmpty(); }
    auto isFull() const { return p.isFull(); }

  private:
    LightQueue<U> &p;
  };
  class Writer {
  public:
    Writer(LightQueue<U> &p) : p(p) { p.has_writer = true; }
    Writer(Writer const &) = delete;
    Writer(Writer &&) = default;
    ~Writer() { p.has_writer = false; }
    auto push(U const &u) noexcept { p.push(u); }
    auto isEmpty() const { return p.isEmpty(); }
    auto isFull() const { return p.isFull(); }

  private:
    LightQueue<U> &p;
  };

public:
  auto isEmpty() const -> bool {
    return write_idx.load(std::memory_order_acquire) <=
           read_idx.load(std::memory_order_acquire);
  }
  auto isFull() const -> bool {
    return write_idx.load(std::memory_order_acquire) -
               read_idx.load(std::memory_order_acquire) >=
           capacity;
  }
  auto getReader() {
    if (has_reader)
      throw std::runtime_error("Reader already exists");
    return std::make_unique<Reader>(*this);
  }
  auto getWriter() {
    if (has_writer)
      throw std::runtime_error("Writer already exists");
    return std::make_unique<Writer>(*this);
  }

protected:
  size_t now_size{0};
  bool has_reader{false};
  bool has_writer{false};
  struct align_wrapper {
    U inner;
  };
  std::array<align_wrapper, capacity> q;
  alignas(64) std::atomic<size_t> read_idx{0};
  alignas(64) std::atomic<size_t> write_idx{0};
  alignas(64) size_t cached_write_idx{0};
  alignas(64) size_t cached_read_idx{0};

protected:
  auto pop() noexcept {
    while (isEmpty()) {
    }
    // while(cached_write_idx <= read_idx.load(std::memory_order_relaxed))
    // [[likely]] {
    //   cached_write_idx = write_idx.load(std::memory_order_acquire);
    // }
    auto new_idx = read_idx.load(std::memory_order_relaxed) + 1;
    auto res = q.at(new_idx % capacity).inner;
    read_idx.store(new_idx, std::memory_order_release);
    return res;
  }
  auto push(U const &u) noexcept {
    while (isFull()) {
    }
    // while(cached_read_idx + capacity <=
    // write_idx.load(std::memory_order_relaxed)) [[likely]] {
    //   cached_read_idx = read_idx.load(std::memory_order_acquire);
    // }
    auto new_idx = write_idx.load(std::memory_order_relaxed) + 1;
    q.at(new_idx % capacity) = align_wrapper{u};
    write_idx.store(new_idx, std::memory_order_release);
  }
};

static_assert(SPSCQueue<LightQueue<int>, int>);