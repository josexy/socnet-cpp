#ifndef SOC_NET_TIMERHEAP_H
#define SOC_NET_TIMERHEAP_H

#include <algorithm>

#include "../../utility/include/timestamp.h"

namespace soc {
namespace net {

using TimeoutCb = std::function<void()>;

struct Timer {
  int fd;
  timestamp ts;
  TimeoutCb cb;
  Timer(int fd, const timestamp &ts, const TimeoutCb &cb)
      : fd(fd), ts(ts), cb(cb) {}
  bool operator<(const Timer &t) const { return t.ts < ts; }
};

class TimerHeap {
public:
  void push(const Timer &value) {
    if (fds_.count(value.fd) == 0) {
      c_.emplace_back(value);
      fds_[value.fd] = n_;
      n_++;
      if (n_ == 1)
        return;
      up_adjust(n_ - 1);
    } else {
      // Timer existed
      size_t i = fds_[value.fd];
      // Reset expired time
      c_[i].ts = value.ts;
      c_[i].cb = value.cb;
      // Try to down adjust
      if (!down_adjust(i)) {
        // Try to up adjust
        up_adjust(i);
      }
    }
  }

  void emplace(int fd, const timestamp &ts, const TimeoutCb &cb) {
    if (fds_.count(fd) == 0) {
      c_.emplace_back(fd, ts, cb);
      fds_[fd] = n_;
      n_++;
      if (n_ == 1)
        return;
      up_adjust(n_ - 1);
    } else {
      // Timer existed
      size_t i = fds_[fd];
      // Reset expired time
      c_[i].ts = ts;
      c_[i].cb = cb;
      // Try to down adjust
      if (!down_adjust(i)) {
        // Try to up adjust
        up_adjust(i);
      }
    }
  }

  void pop() {
    if (empty())
      std::exception();
    del(0);
  }

  Timer &top() {
    if (empty())
      throw std::exception();
    return c_[0];
  }

  bool empty() const noexcept { return size() == 0; }
  size_t size() const noexcept { return n_; }
  void clear() noexcept {
    c_.clear();
    fds_.clear();
  }

  void adjust(int fd, const timestamp &ts) {
    if (auto x = fds_.find(fd); x != fds_.end()) {
      // reset expired time
      c_[x->second].ts = ts;
      down_adjust(x->second);
    }
  }

  void erase(int fd) {
    if (auto x = fds_.find(fd); x != fds_.end()) {
      del(x->second);
    }
  }

private:
  void del(size_t i) {
    if (empty())
      return;
    std::swap(c_[i], c_[size() - 1]);
    n_--;
    // Try to down adjust
    if (!down_adjust(i)) {
      // otherwise, try to up adjust
      up_adjust(i);
    }
    // Erase back Timer
    fds_.erase(c_.back().fd);
    c_.pop_back();
  }

  // for insert
  void up_adjust(size_t i) {
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
  bool down_adjust(size_t i) {
    size_t x = i;
    size_t child = i * 2 + 1;
    const Timer v = c_[i];
    while (true) {
      if (child >= size())
        break;

      // choose a min expired timer
      if (child + 1 < size() && c_[child] < c_[child + 1])
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
  std::unordered_map<int, int> fds_;
  size_t n_ = 0;
};

} // namespace net
} // namespace soc

#endif
