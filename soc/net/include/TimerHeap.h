#ifndef SOC_NET_TIMERHEAP_H
#define SOC_NET_TIMERHEAP_H

#include "TimeStamp.h"
#include <algorithm>
#include <mutex>

namespace soc {
namespace net {

using TimeoutCallback = std::function<void()>;

struct Timer {
  int fd;
  TimeStamp timestamp;
  TimeoutCallback callback;

  Timer() {}
  explicit Timer(int fd, const TimeStamp &ts, const TimeoutCallback &cb)
      : fd(fd), timestamp(ts), callback(cb) {}
  bool operator<(const Timer &other) const {
    return other.timestamp < timestamp;
  }
};

class TimerHeap {
public:
  ~TimerHeap() { clear(); }

  void push(const Timer &timer) {
    std::lock_guard<std::mutex> locker(mutex_);
    if (fds_.count(timer.fd) == 0) {
      c_.emplace_back(timer);
      fds_[timer.fd] = n_;
      n_++;
      if (n_ == 1)
        return;
      upAdjust(n_ - 1);
    } else {
      // Timer existed
      size_t i = fds_[timer.fd];
      // Reset expired time
      c_[i].timestamp = timer.timestamp;
      c_[i].callback = timer.callback;
      // Try to down adjust
      if (!downAdjust(i)) {
        // Try to up adjust
        upAdjust(i);
      }
    }
  }

  void pop() {
    std::lock_guard<std::mutex> locker(mutex_);
    if (n_ == 0)
      throw std::exception();
    del(0);
  }
  Timer &top() {
    std::lock_guard<std::mutex> locker(mutex_);
    if (n_ == 0)
      throw std::exception();
    return c_[0];
  }
  bool empty() const noexcept {
    std::lock_guard<std::mutex> locker(mutex_);
    return n_ == 0;
  }
  size_t size() const noexcept {
    std::lock_guard<std::mutex> locker(mutex_);
    return n_;
  }
  void clear() {
    std::lock_guard<std::mutex> locker(mutex_);
    c_.clear();
    fds_.clear();
  }

  void adjust(const Timer &timer) {
    std::lock_guard<std::mutex> locker(mutex_);
    if (auto x = fds_.find(timer.fd); x != fds_.end()) {
      // reset expired time
      c_[x->second].timestamp = timer.timestamp;
      downAdjust(x->second);
    }
  }

private:
  void del(size_t i) {
    if (n_ == 0)
      return;
    std::swap(c_[i], c_[n_ - 1]);
    n_--;
    // Try to down adjust
    if (!downAdjust(i)) {
      // otherwise, try to up adjust
      upAdjust(i);
    }
    // Erase back Timer
    fds_.erase(c_.back().fd);
    c_.pop_back();
  }

  // for insert
  void upAdjust(size_t i) {
    size_t parent = (i - 1) / 2;

    // last timer
    const Timer v = c_[i];
    while (true) {
      if (i <= 0)
        break;
      // if parent expired time < current expired time
      // than break loop
      if (v < c_[parent])
        break;
      /// otherwise, move down the parent timer
      fds_[c_[parent].fd] = i;
      c_[i] = c_[parent];

      i = parent;
      parent = (i - 1) / 2;
    }
    c_[i] = v;
    fds_[c_[i].fd] = i;
  }

  // for delete
  bool downAdjust(size_t i) {
    size_t x = i;
    size_t child = i * 2 + 1;
    const Timer v = c_[i];
    while (true) {
      if (child >= n_)
        break;

      // choose a min expired timer
      if (child + 1 < n_ && c_[child] < c_[child + 1])
        child++;

      // if current expired timer < the child expired tiemr
      // then break loop
      if (c_[child] < v)
        break;
      // otherwise, move up the child timer
      fds_[c_[child].fd] = i;
      c_[i] = c_[child];

      i = child;
      child = i * 2 + 1;
    }
    c_[i] = v;
    fds_[c_[i].fd] = i;
    return x > i;
  }

private:
  std::vector<Timer> c_;
  size_t n_ = 0;
  std::unordered_map<int, int> fds_;
  mutable std::mutex mutex_;
};

} // namespace net
} // namespace soc

#endif
