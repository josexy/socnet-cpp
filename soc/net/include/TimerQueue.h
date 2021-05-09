#ifndef SOC_NET_TIMERQUEUE_H
#define SOC_NET_TIMERQUEUE_H

#include "TimerHeap.h"

namespace soc {
namespace net {

class TimerQueue {
public:
  TimerQueue();

  void run_once(const timestamp &t);
  void run_every(const timestamp &t);

  int fd() const { return tmfd_; }
  void adjust(int fd, const timestamp &t);
  void add(int fd, const timestamp &ts, const TimeoutCb &cb);
  decltype(auto) expired_timer() { return timer_heap_.top(); }

  void handle_timeout();

private:
  int tmfd_;
  TimerHeap timer_heap_;
  TimeoutCb cb_;
};
} // namespace net
} // namespace soc
#endif
