#include "../include/TimerQueue.h"

#include <sys/timerfd.h>
#include <unistd.h>

using namespace soc::net;

TimerQueue::TimerQueue() : tmfd_(::timerfd_create(CLOCK_REALTIME, 0)) {}

// run once
void TimerQueue::run_once(const timestamp &t) {
  uint64_t second = t.second();
  uint64_t nanosecond = (t.microsecond() - t.second() * 1000000) * 100;
  timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  itimerspec itsp;
  itsp.it_value.tv_sec = now.tv_sec + second;
  itsp.it_value.tv_nsec = now.tv_nsec + nanosecond;
  itsp.it_interval.tv_sec = 0;
  itsp.it_interval.tv_nsec = 0;

  timerfd_settime(tmfd_, TFD_TIMER_ABSTIME, &itsp, nullptr);
}

// run every
void TimerQueue::run_every(const timestamp &t) {
  uint64_t second = t.second();
  uint64_t nanosecond = (t.microsecond() - t.second() * 1000000) * 100;
  timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  itimerspec itsp;
  itsp.it_value.tv_sec = now.tv_sec + second;
  itsp.it_value.tv_nsec = now.tv_nsec + nanosecond;
  itsp.it_interval.tv_sec = second;
  itsp.it_interval.tv_nsec = nanosecond;
  timerfd_settime(tmfd_, TFD_TIMER_ABSTIME, &itsp, nullptr);
}

void TimerQueue::adjust(int fd, const timestamp &t) {
  timer_heap_.adjust(fd, t);
}

void TimerQueue::add(int fd, const timestamp &ts, const TimeoutCb &cb) {
  timer_heap_.emplace(fd, std::forward<const timestamp &>(ts),
                      std::forward<const TimeoutCb &>(cb));
}

void TimerQueue::handle_timeout() {
  uint64_t one;
  ::read(tmfd_, &one, sizeof(one));
  timestamp now = timestamp::now();
  while (!timer_heap_.empty()) {
    auto expired = timer_heap_.top();
    if (expired.ts <= now) {
      expired.cb();
      timer_heap_.pop();
    } else {
      break;
    }
  }
}
