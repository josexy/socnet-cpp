#ifndef SOC_HTTP_HTTPHEADER_H
#define SOC_HTTP_HTTPHEADER_H

#include "../../net/include/Buffer.h"
#include "../../utility/include/EncodeUtil.h"
#include "HttpMap.h"
#include <map>

namespace soc {
namespace http {

class HttpHeader : public Map<std::string, std::string> {
public:
  using Callback =
      std::function<void(const std::string &, const std::string &)>;
  using Mutex = std::shared_mutex;
  using ReadLock = std::shared_lock<Mutex>;
  using WriteLock = std::unique_lock<Mutex>;

  HttpHeader();
  explicit HttpHeader(std::string_view header);
  int parse(std::string_view header);

  void add(const HttpHeader &header);
  void add(const std::string &key, const std::string &value) override;
  void remove(const std::string &key) override;
  std::optional<std::string> get(const std::string &key) const override;
  bool contain(const std::string &key) const noexcept override;
  size_t size() const noexcept override {
    ReadLock locker(mutex_);
    return headers_.size();
  }
  bool empty() const noexcept override {
    ReadLock locker(mutex_);
    return headers_.empty();
  }
  void clear() override {
    WriteLock locker(mutex_);
    headers_.clear();
  }

  std::string toString() const;

  void forEach(const Callback &) const override;

  void store(net::Buffer *sender);

protected:
  void add0(const std::string &key, const std::string &value);

private:
  std::multimap<std::string, std::string> headers_;
  mutable Mutex mutex_;
};
} // namespace http
} // namespace soc
#endif
