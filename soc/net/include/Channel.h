#ifndef SOC_NET_CHANNEL_H
#define SOC_NET_CHANNEL_H

#include "Buffer.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/uio.h>
#include <unistd.h>

namespace soc {
namespace net {

enum ChannelType { Raw, Ssl };
static constexpr const size_t kBufferSize = 4096;

class Channel {
public:
  Channel(bool support_sendfile)
      : sendfile_(support_sendfile), mmap_obj_(nullptr), sfile_obj_(nullptr),
        send_bytes_(0), remain_bytes_(0), wsend_bytes_(0) {
    buffer_ = new char[kBufferSize];
  }

  virtual ~Channel() {
    if (buffer_)
      delete[] buffer_;
    buffer_ = nullptr;
  }

  virtual int read() = 0;
  virtual int write(bool *) = 0;
  virtual void close() = 0;
  virtual int getError(int) = 0;
  virtual ChannelType getType() = 0;
  virtual bool supportSendFile() const { return sendfile_; }

  class MMap {
  public:
    explicit MMap(int infd, size_t size) : fd_(infd), size_(size) {
      addr_ = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
      if (addr_ == MAP_FAILED) {
        addr_ = nullptr;
        size_ = 0;
      }
    }
    ~MMap() {
      if (addr_)
        ::munmap(addr_, size_);
      addr_ = nullptr;
    }
    char *getAddress() const noexcept { return static_cast<char *>(addr_); }
    int getFd() const noexcept { return fd_; }
    size_t getSize() const noexcept { return size_; }

  private:
    void *addr_;
    int fd_;
    size_t size_;
  };

  class SendFile {
  public:
    explicit SendFile(int infd, int outfd, size_t size)
        : infd_(infd), outfd_(outfd), size_(size) {}
    ~SendFile() { ::close(infd_); }

    int sendFile() {
      // If offset is NULL, then data will be read from in_fd starting at the
      // file offset, and the file offset will be updated by the call.
      return ::sendfile(outfd_, infd_, nullptr, size_);
    }

    int getInFd() const noexcept { return infd_; }
    int getOutFd() const noexcept { return outfd_; }
    size_t getSize() const noexcept { return size_; }

  private:
    int infd_;
    int outfd_;
    size_t size_;
  };

  MMap *createMMapObject(int infd, size_t size);
  virtual SendFile *createSendFileObject(int infd, size_t size) = 0;

  MMap *getMMapObject() const { return mmap_obj_; }
  SendFile *getSendFileObject() const { return sfile_obj_; }

  void removeMMapObject() {
    if (mmap_obj_)
      delete mmap_obj_;
    mmap_obj_ = nullptr;
  }
  void removeSendFileObject() {
    if (sfile_obj_)
      delete sfile_obj_;
    sfile_obj_ = nullptr;
  }

  Buffer *getSender() { return &sender_; }
  Buffer *getRecver() { return &recver_; }
  void reset();

protected:
  bool sendfile_;

  MMap *mmap_obj_;
  SendFile *sfile_obj_;
  char *buffer_;

  Buffer sender_;
  Buffer recver_;

  long send_bytes_;
  long remain_bytes_;
  long wsend_bytes_;
};

class RawChannel : public Channel {
public:
  explicit RawChannel(int fd, bool sf) : Channel(sf), fd_(fd), closed_(false) {}

  virtual ~RawChannel() {
    if (!closed_)
      close();
  }

  int read() override;
  int write(bool *) override;
  void close() override;
  int getError(int) override;
  ChannelType getType() override { return ChannelType::Raw; }

  SendFile *createSendFileObject(int fd, size_t size) override;

private:
  int fd_;
  bool closed_;
};

class SslChannel : public RawChannel {
public:
  explicit SslChannel(SSL *ssl, int fd) : RawChannel(fd, false), ssl_(ssl) {}
  ~SslChannel() {
    if (ssl_)
      this->close();
  }

  int read() override;
  int write(bool *) override;
  void close() override;
  int getError(int) override;
  ChannelType getType() override { return ChannelType::Ssl; }

  SSL *ssl() const noexcept { return ssl_; }

private:
  SSL *ssl_;
};
} // namespace net
} // namespace soc

#endif