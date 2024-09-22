#pragma once

#include <concepts>

template <class T, class U>
concept SPSCQueueReader = requires(T t) {
  { t.pop() } -> std::same_as<U>;
};

template <class T, class U>
concept SPSCQueueWriter = requires(T t, U u) { t.push(u); };

template <class T, class U>
concept SPSCQueue = requires(T t) {
  requires SPSCQueueReader<decltype(*t.getReader()), U>;
  requires SPSCQueueWriter<decltype(*t.getWriter()), U>;
};
