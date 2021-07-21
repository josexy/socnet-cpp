#include "../include/TimerQueue.h"
#include <sys/timerfd.h>
#include <unistd.h>

using namespace soc::net;

TimerQueue::TimerQueue()
    : tmfd_(::timerfd_create(CLOCK_REALTIME, 0)), timer_heap_(new TimerHeap) {}

// run once
void TimerQueue::runOnce(const TimeStamp &t) {
  uint64_t second = t.second();
  uint64_t nanosecond = (t.microsecond() - t.second() * 1000000) * 100;
  timespec now;
  ::clock_gettime(CLOCK_REALTIME, &now);
  itimerspec itsp;
  itsp.it_value.tv_sec = now.tv_sec + second;
  itsp.it_value.tv_nsec = now.tv_nsec + nanosecond;
  itsp.it_interval.tv_sec = 0;
  itsp.it_interval.tv_nsec = 0;

  ::timerfd_settime(tmfd_, TFD_TIMER_ABSTIME, &itsp, nullptr);
}

// run every
void TimerQueue::runEvery(const TimeStamp &t) {
  uint64_t second = t.second();
  uint64_t nanosecond = (t.microsecond() - t.second() * 1000000) * 100;
  timespec now;
  ::clock_gettime(CLOCK_REALTIME, &now);
  itimerspec itsp;
  itsp.it_value.tv_sec = now.tv_sec + second;
  itsp.it_value.tv_nsec = now.tv_nsec + nanosecond;
  itsp.it_interval.tv_sec = second;
  itsp.it_interval.tv_nsec = nanosecond;

  ::timerfd_settime(tmfd_, TFD_TIMER_ABSTIME, &itsp, nullptr);
}

void TimerQueue::add(const Timer &timer) { timer_heap_->push(timer); }

void TimerQueue::adjust(const Timer &timer) { timer_heap_->adjust(timer); }

void TimerQueue::handleTimeout() {
  uint64_t one;
  ::read(tmfd_, &one, sizeof(one));

  TimeStamp now = TimeStamp::now();
  while (!timer_heap_->empty()) {
    auto expired = timer_heap_->top();
    if (expired.timestamp <= now) {
      expired.callback();
      timer_heap_->pop();
    } else {
      break;
    }
  }
}
