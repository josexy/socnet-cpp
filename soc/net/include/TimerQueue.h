#ifndef SOC_NET_TIMERQUEUE_H
#define SOC_NET_TIMERQUEUE_H

#include "TimerHeap.h"
#include <memory>

namespace soc {
namespace net {

class TimerQueue {
public:
  TimerQueue();

  void runOnce(const TimeStamp &t);
  void runEvery(const TimeStamp &t);

  int fd() const { return tmfd_; }
  const Timer &expiredTimer() { return timer_heap_->top(); }

  void add(const Timer &);
  void adjust(const Timer &);

  void handleTimeout();

private:
  int tmfd_;
  std::unique_ptr<TimerHeap> timer_heap_;
  TimeoutCallback cb_;
};
} // namespace net
} // namespace soc
#endif
