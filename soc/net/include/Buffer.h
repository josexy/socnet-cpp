#ifndef SOC_NET_BUFFER_H
#define SOC_NET_BUFFER_H

#include <errno.h>
#include <string>
#include <vector>

namespace soc {
namespace net {

class Buffer {
public:
  explicit Buffer(size_t init_size = 4096)
      : buffer_(init_size), rindex_(0), windex_(0) {}

  void reset() { rindex_ = windex_ = 0; }

  size_t readable() const noexcept { return windex_ - rindex_; }
  size_t writable() const noexcept { return buffer_.size() - windex_; }

  const char *peek() noexcept { return beginRead(); }
  char *beginWrite() noexcept { return begin() + windex_; }
  char *beginRead() noexcept { return begin() + rindex_; }

  void retiredAll() noexcept { rindex_ = windex_ = 0; }
  void retired(size_t len) noexcept {
    if (len < readable())
      rindex_ += len;
    else
      retiredAll();
  }

  template <class T>
  void append(const typename std::vector<T>::iterator &begin,
              const typename std::vector<T>::iterator &end) {
    int len = end - begin;
    if (len <= 0)
      return;
    ensureWritable(len);
    std::copy(begin, end, beginWrite());
    hasWritten(len);
  }

  void append(const char *data, size_t len) {
    if (len <= 0)
      return;
    ensureWritable(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  void append(std::string_view data) { append(data.data(), data.size()); }

  void ensureWritable(size_t len) {
    if (writable() < len)
      makeSpace(len);
  }

  void hasWritten(size_t len) { windex_ += len; }

private:
  char *begin() noexcept { return buffer_.data(); }

  void makeSpace(size_t len) {
    if (rindex_ + writable() < len) {
      buffer_.resize(windex_ + len + 1);
    } else {
      size_t rlen = readable();
      std::copy(beginRead(), beginWrite(), begin());
      rindex_ = 0;
      windex_ = rindex_ + rlen;
    }
  }

  std::vector<char> buffer_;
  size_t rindex_;
  size_t windex_;
};
} // namespace net
} // namespace soc

#endif
