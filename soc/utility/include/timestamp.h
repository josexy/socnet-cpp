#ifndef SOC_UTILITY_TIMESTAMP_H
#define SOC_UTILITY_TIMESTAMP_H

#include <sys/time.h>
#include <time.h>

#include <string>
#define __LOGGER_DATETIME 1

namespace soc {
class timestamp {
public:
  explicit timestamp(uint64_t microsecond = 0) : microsecond_(microsecond) {}

  uint64_t second() const noexcept { return microsecond_ / 1000000; }
  uint64_t millsecond() const noexcept { return microsecond_ / 1000; }
  uint64_t microsecond() const noexcept { return microsecond_; }
  uint64_t nanosecond() const noexcept { return microsecond_ * 1000; }

  timestamp &operator+(const timestamp &t) noexcept {
    microsecond_ += t.microsecond_;
    return *this;
  }

  timestamp &operator-(const timestamp &t) noexcept {
    microsecond_ -= t.microsecond_;
    return *this;
  }

  timestamp &operator+(int second) noexcept {
    microsecond_ += (second * 1000000);
    return *this;
  }

  timestamp &operator-(int second) noexcept {
    microsecond_ -= (second * 1000000);
    return *this;
  }

  bool operator<(const timestamp &t) const noexcept {
    return microsecond_ < t.microsecond_;
  }

  bool operator<=(const timestamp &t) const noexcept {
    return microsecond_ <= t.microsecond_;
  }

  bool operator>(const timestamp &t) const noexcept {
    return microsecond_ > t.microsecond_;
  }
  bool operator==(const timestamp &t) const noexcept {
    return microsecond_ == t.microsecond_;
  }

  static timeval now_tv() {
    timeval tv;
    ::gettimeofday(&tv, nullptr);
    return tv;
  }

  static timestamp now() {
    timeval tv = now_tv();
    uint64_t microsecond = tv.tv_sec * 1000000 + tv.tv_usec;
    return timestamp(microsecond);
  }

  static timestamp second(int sec) { return timestamp(sec * 1000000); }
  static timestamp millsecond(int msec) { return timestamp(msec * 1000); }
  static timestamp microsecond(int microsec) { return timestamp(microsec); }
  static timestamp nanosecond(int nsec) { return timestamp(nsec / 1000); }
  static timestamp max_timestamp() noexcept {
    return timestamp(std::numeric_limits<uint64_t>::max());
  }

  static timestamp now_second(int sec) {
    return timestamp::now() + second(sec);
  }
  static timestamp now_msecond(int msec) {
    return timestamp::now() + millsecond(msec);
  }
  static timestamp now_nanosecond(int nsec) {
    return timestamp::now() + nanosecond(nsec);
  }

  static std::string fmt_time_t(time_t t) {
    char buf[25]{0};
    ::strftime(buf, 25, "%Y-%m-%d %H:%M:%S", ::localtime(&t));
    return buf;
  }

  static std::string now_s() { return fmt_time_t(::time(nullptr)); }

  static std::string server_date() {
    char buffer[50]{0};
    time_t t = ::time(nullptr);
    ::strftime(buffer, 50, "%a, %d %b %Y %H:%M:%S GMT", ::gmtime(&t));
    return buffer;
  }

private:
  uint64_t microsecond_;
};

} // namespace soc
#endif
