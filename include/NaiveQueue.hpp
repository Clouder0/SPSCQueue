#pragma once
#include "queue.hpp"
#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>

template <class U, size_t capacity = 1024> class NaiveQueue {
  class Reader {
  public:
    Reader(NaiveQueue<U> &p) : p(p) { p.has_reader = true; }
    Reader(Reader const &) = delete;
    Reader(Reader &&) = default;
    ~Reader() { p.has_reader = false; }
    auto front() {
      std::lock_guard lock(p.m);
      return p.q.at(read_idx % capacity);
    }
    auto pop() {
      std::unique_lock lock(p.m);
      if (p.isEmpty()) {
        p.cv.wait(lock, [&] { return !p.isEmpty(); });
      }
      --p.now_size;
      if (p.now_size == capacity - 1)
        p.cv.notify_one();
      return p.q.at(read_idx++ % capacity);
    }
    auto isEmpty() const { return p.isEmpty(); }
    auto isFull() const { return p.isFull(); }

  private:
    NaiveQueue<U> &p;
    size_t read_idx;
  };
  class Writer {
  public:
    Writer(NaiveQueue<U> &p) : p(p) { p.has_writer = true; }
    Writer(Writer const &) = delete;
    Writer(Writer &&) = default;
    ~Writer() { p.has_writer = false; }

    auto push(U const &u) {
      std::unique_lock lock(p.m);
      if (p.isFull()) {
        p.cv.wait(lock, [&] { return !p.isFull(); });
      }
      p.q.at(write_idx++ % capacity) = u;
      ++p.now_size;
      if (p.now_size == 1)
        p.cv.notify_one();
    }
    auto isEmpty() const { return p.isEmpty(); }
    auto isFull() const { return p.isFull(); }

  private:
    NaiveQueue<U> &p;
    size_t write_idx;
  };

public:
  auto isEmpty() const -> bool { return now_size == 0; }
  auto isFull() const -> bool { return now_size == capacity; }
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
  std::array<U, capacity> q;
  std::mutex m;
  std::condition_variable cv;
  bool has_reader{false};
  bool has_writer{false};
};

static_assert(SPSCQueue<NaiveQueue<int>, int>);