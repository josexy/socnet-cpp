#ifndef SOC_NET_BUFFER_H
#define SOC_NET_BUFFER_H

#include <errno.h>
#include <string>
#include <sys/mman.h>
#include <sys/uio.h>
#include <vector>

namespace soc {
namespace net {

class Buffer {
public:
  explicit Buffer(size_t init_size = 4096)
      : buffer_(init_size), rindex_(0), windex_(0), mapping_addr_(nullptr),
        mapping_size_(0), switch_mapping_(false) {}

  ~Buffer() { reset(); }

  void reset() {
    rindex_ = windex_ = 0;
    if (mapping_addr_)
      ::munmap(mapping_addr_, mapping_size_);
    mapping_addr_ = nullptr;
    mapping_size_ = 0;
    switch_mapping_ = false;
  }

  size_t readable() const noexcept { return windex_ - rindex_; }
  size_t writable() const noexcept { return buffer_.size() - windex_; }
  size_t mappingSize() const noexcept { return mapping_size_; }

  const char *peek() noexcept { return beginRead(); }
  char *beginWrite() noexcept { return begin() + windex_; }
  char *mappingAddr() noexcept { return mapping_addr_; }
  char *beginRead() noexcept {
    if (switch_mapping_ && mapping_addr_)
      return mapping_addr_ + rindex_;
    else
      return begin() + rindex_;
  }

  bool isSwitchMapping() const noexcept { return switch_mapping_; }
  void switchMapping(bool is) noexcept { switch_mapping_ = is; }

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

  void appendMapping(std::string_view mapping) {
    appendMapping(const_cast<char *>(mapping.data()), mapping.size());
  }

  void appendMapping(char *mapping, size_t len) {
    mapping_addr_ = mapping;
    mapping_size_ = len;
  }

  void ensureWritable(size_t len) {
    if (writable() < len)
      makeSpace(len);
  }

  void hasWritten(size_t len) { windex_ += len; }

  // Not used
  int readFd(int fd, int *err) {
    char buff[65535];
    struct iovec iov[2];
    size_t w_len = writable();

    iov[0].iov_base = begin() + windex_;
    iov[0].iov_len = w_len;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    int len = ::readv(fd, iov, 2);
    if (len < 0) {
      *err = errno;
    } else if (static_cast<size_t>(len) <= w_len) {
      windex_ += len;
    } else {
      windex_ = buffer_.size();
      append(buff, len - w_len);
    }
    return len;
  }

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

  char *mapping_addr_;
  size_t mapping_size_;
  bool switch_mapping_;
};
} // namespace net
} // namespace soc

#endif
