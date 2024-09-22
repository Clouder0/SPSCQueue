#pragma once

#include "queue.hpp"
#include <array>
#include <atomic>
#include <memory>

template <class U, size_t capacity = 1024> class SizeQueue {
  class Reader {
  public:
    Reader(SizeQueue<U> &p) : p(p) { p.has_reader = true; }
    Reader(Reader const &) = delete;
    Reader(Reader &&) = default;
    ~Reader() { p.has_reader = false; }
    auto front() { return p.q.at(p.read_idx % capacity).inner; }
    auto pop() noexcept { return p.pop(); }
    auto isEmpty() const { return p.isEmpty(); }
    auto isFull() const { return p.isFull(); }

  private:
    SizeQueue<U> &p;
  };
  class Writer {
  public:
    Writer(SizeQueue<U> &p) : p(p) { p.has_writer = true; }
    Writer(Writer const &) = delete;
    Writer(Writer &&) = default;
    ~Writer() { p.has_writer = false; }
    auto push(U const &u) noexcept { p.push(u); }
    auto isEmpty() const { return p.isEmpty(); }
    auto isFull() const { return p.isFull(); }

  private:
    SizeQueue<U> &p;
  };

public:
  auto isEmpty() const -> bool {
    return qsize.load(std::memory_order_acquire) == 0;
  }
  auto isFull() const -> bool {
    return qsize.load(std::memory_order_acquire) == capacity;
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
  std::atomic<size_t> qsize{0};
  size_t read_idx, write_idx;

protected:
  auto pop() noexcept {
    while (isEmpty()) {
    }
    auto res = q.at(read_idx++ % capacity).inner;
    qsize.fetch_sub(1, std::memory_order_release);
    return res;
  }
  auto push(U const &u) noexcept {
    while (isFull()) {
    }
    q.at(write_idx++ % capacity).inner = u;
    qsize.fetch_add(1, std::memory_order_release);
  }
};

static_assert(SPSCQueue<SizeQueue<int>, int>);