#ifndef SOC_HTTP_HTTPMAP_H
#define SOC_HTTP_HTTPMAP_H

#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace soc {
namespace http {

template <class Key, class Value> class Map {
public:
  using Callback = std::function<void(const Key &, const Value &)>;

  virtual ~Map() {}
  virtual void add(const Key &, const Value &) = 0;
  virtual void remove(const Key &) = 0;
  virtual std::optional<Value> get(const Key &) const = 0;
  virtual bool contain(const Key &) const noexcept = 0;
  virtual size_t size() const noexcept = 0;
  virtual bool empty() const noexcept = 0;
  virtual void forEach(const Callback &) const = 0;
  virtual void clear() = 0;
};

template <class Key, class Value, class _Mutex = std::shared_mutex,
          template <class M> class _ReadLock = std::shared_lock,
          template <class M> class _WriteLock = std::unique_lock>
class HttpMap : public Map<Key, Value> {
public:
  using Callback = std::function<void(const Key &, const Value &)>;
  using EachCallback = std::function<bool(const Key &, const Value &)>;
  using Mutex = _Mutex;
  using ReadLock = _ReadLock<Mutex>;
  using WriteLock = _WriteLock<Mutex>;

  virtual ~HttpMap() { clear(); }
  virtual void add(const Key &key, const Value &value) {
    WriteLock locker(mutex_);
    map_[std::forward<const Key &>(key)] = std::forward<const Value &>(value);
  }
  virtual void remove(const Key &key) {
    WriteLock locker(mutex_);
    if (auto it = map_.find(key); it != map_.end())
      map_.erase(it);
  }
  virtual std::optional<Value> get(const Key &key) const {
    ReadLock locker(mutex_);
    if (auto it = map_.find(key); it != map_.end())
      return std::make_optional(it->second);
    return std::nullopt;
  }
  virtual bool contain(const Key &key) const noexcept {
    ReadLock locker(mutex_);
    return map_.find(key) != map_.end();
  }
  virtual size_t size() const noexcept {
    ReadLock locker(mutex_);
    return map_.size();
  }
  virtual bool empty() const noexcept {
    ReadLock locker(mutex_);
    return map_.empty();
  }
  virtual Value &operator[](const Key &key) {
    WriteLock locker(mutex_);
    return map_[key];
  }
  virtual void forEach(const Callback &callback) const {
    ReadLock locker(mutex_);
    for (const auto &[k, v] : map_)
      callback(k, v);
  }
  virtual void clear() {
    WriteLock locker(mutex_);
    map_.clear();
  }
  void each(const EachCallback &callback) {
    ReadLock locker(mutex_);
    for (const auto &[k, v] : map_)
      if (callback(k, v))
        break;
  }

protected:
  std::unordered_map<Key, Value> map_;
  mutable Mutex mutex_;
};

} // namespace http
} // namespace soc

#endif