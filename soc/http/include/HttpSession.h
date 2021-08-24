#ifndef SOC_HTTP_HTTPSESSION_H
#define SOC_HTTP_HTTPSESSION_H

#include "HttpMap.h"
#include <any>

namespace soc {
namespace http {
class HttpSession : public HttpMap<std::string, std::any> {
public:
  enum Status { New, Accessed, Destroy };

  HttpSession(const std::string &id, int interval)
      : id_(id), interval_(interval), status_(Status::New) {}
  ~HttpSession() {}

  const std::string &getId() const noexcept {
    ReadLock locker(mutex_);
    return id_;
  }
  bool isNew() const noexcept {
    ReadLock locker(mutex_);
    return status_ == Status::New;
  }
  bool isAccessed() const noexcept {
    ReadLock locker(mutex_);
    return status_ == Status::Accessed;
  }
  bool isDestroy() const noexcept {
    ReadLock locker(mutex_);
    return status_ == Status::Destroy;
  }
  Status getStatus() const noexcept {
    ReadLock locker(mutex_);
    return status_;
  }
  int getMaxInactiveInterval() const noexcept {
    ReadLock locker(mutex_);
    return interval_;
  }
  void invalidate() {
    WriteLock locker(mutex_);
    status_ = Status::Destroy;
  }
  void setStatus(Status s) {
    WriteLock locker(mutex_);
    status_ = s;
  }
  template <class V> void setValue(const std::string &key, const V &value) {
    add(key, std::forward<const V &>(value));
  }

  template <class V> const V *getValue(const std::string &key) const {
    ReadLock locker(mutex_);
    if (auto x = map_.find(key); x != map_.end()) {
      if (!x->second.has_value())
        return nullptr;
      try {
        return &std::any_cast<const V &>(x->second);
      } catch (std::bad_any_cast &) {
      }
    }
    return nullptr;
  }

private:
  std::string id_;
  int interval_;
  Status status_;
};

} // namespace http
} // namespace soc

#endif