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

  int getFd() const { return tmfd_; }
  const Timer &getExpiredTimer() { return timer_heap_->top(); }

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
