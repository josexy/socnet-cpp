#ifndef SOC_NET_TIMESTAMP_H
#define SOC_NET_TIMESTAMP_H

#include <limits>
#include <string>
#include <sys/time.h>
#include <time.h>

namespace soc {
namespace net {

class TimeStamp {
public:
  explicit TimeStamp(uint64_t microsecond = 0) : microsecond_(microsecond) {}

  uint64_t second() const noexcept { return microsecond_ / 1000000; }
  uint64_t millsecond() const noexcept { return microsecond_ / 1000; }
  uint64_t microsecond() const noexcept { return microsecond_; }
  uint64_t nanosecond() const noexcept { return microsecond_ * 1000; }

  TimeStamp &operator+(const TimeStamp &t) noexcept {
    microsecond_ += t.microsecond_;
    return *this;
  }

  TimeStamp &operator-(const TimeStamp &t) noexcept {
    microsecond_ -= t.microsecond_;
    return *this;
  }

  TimeStamp &operator+(int second) noexcept {
    microsecond_ += (second * 1000000);
    return *this;
  }

  TimeStamp &operator-(int second) noexcept {
    microsecond_ -= (second * 1000000);
    return *this;
  }

  bool operator<(const TimeStamp &t) const noexcept {
    return microsecond_ < t.microsecond_;
  }

  bool operator<=(const TimeStamp &t) const noexcept {
    return microsecond_ <= t.microsecond_;
  }

  bool operator>(const TimeStamp &t) const noexcept {
    return microsecond_ > t.microsecond_;
  }
  bool operator==(const TimeStamp &t) const noexcept {
    return microsecond_ == t.microsecond_;
  }

  static TimeStamp now() {
    timeval tv;
    ::gettimeofday(&tv, nullptr);
    uint64_t microsecond = tv.tv_sec * 1000000 + tv.tv_usec;
    return TimeStamp(microsecond);
  }

  static TimeStamp second(int sec) { return TimeStamp(sec * 1000000); }
  static TimeStamp millsecond(int msec) { return TimeStamp(msec * 1000); }
  static TimeStamp microsecond(int microsec) { return TimeStamp(microsec); }
  static TimeStamp nanosecond(int nsec) { return TimeStamp(nsec / 1000); }
  static TimeStamp max() noexcept {
    return TimeStamp(std::numeric_limits<uint64_t>::max());
  }

  static TimeStamp nowSecond(int sec) { return TimeStamp::now() + second(sec); }
  static TimeStamp nowMsecond(int msec) {
    return TimeStamp::now() + millsecond(msec);
  }
  static TimeStamp nowNanosecond(int nsec) {
    return TimeStamp::now() + nanosecond(nsec);
  }

  static std::string serverDate() {
    char buffer[50]{0};
    time_t t = ::time(nullptr);
    ::strftime(buffer, 50, "%a, %d %b %Y %H:%M:%S GMT", ::gmtime(&t));
    return buffer;
  }

private:
  uint64_t microsecond_;
};

} // namespace net
} // namespace soc
#endif
