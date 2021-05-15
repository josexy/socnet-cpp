#ifndef SOC_NET_BUFFER_H
#define SOC_NET_BUFFER_H

#include <errno.h>
#include <sys/uio.h>

#include <vector>
namespace soc {
namespace net {

class Buffer {
public:
  explicit Buffer(size_t init_size = 2048)
      : buffer_(init_size), rindex_(0), windex_(0) {}

  size_t readable() const noexcept { return windex_ - rindex_; }
  size_t writable() const noexcept { return buffer_.size() - windex_; }

  const char *begin() const noexcept { return &*buffer_.begin(); }
  char *begin() noexcept { return &*buffer_.begin(); }
  char *begin_write() noexcept { return &*buffer_.begin() + windex_; }
  const char *peek() const noexcept { return begin() + rindex_; }

  void retired_all() { rindex_ = windex_ = 0; }
  void retired(size_t len) noexcept {
    if (len < readable())
      rindex_ += len;
    else
      retired_all();
  }

  template <class T>
  void append(const typename std::vector<T>::iterator &begin,
              const typename std::vector<T>::iterator &end) {
    int len = end - begin;
    if (len <= 0)
      return;
    ensure_writable(len);
    std::copy(begin, end, buffer_.data() + windex_);
    has_written(len);
  }

  void append(const char *data, size_t len) {
    if (len <= 0)
      return;
    ensure_writable(len);
    std::copy(data, data + len, buffer_.data() + windex_);
    has_written(len);
  }

  void ensure_writable(size_t len) {
    if (writable() < len)
      make_space(len);
  }

  void has_written(size_t len) { windex_ += len; }

  int read_fd(int fd, int *err) {
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
  void make_space(size_t len) {
    if (rindex_ + writable() < len) {
      buffer_.resize(windex_ + len + 1);
    } else {
      size_t rlen = readable();
      std::copy(begin() + rindex_, begin() + windex_, begin());
      rindex_ = 0;
      windex_ = rindex_ + rlen;
    }
  }

private:
  std::vector<char> buffer_;
  size_t rindex_;
  size_t windex_;
};
} // namespace net
} // namespace soc

#endif
