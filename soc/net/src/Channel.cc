#include "../include/Channel.h"
using namespace soc::net;

void Channel::reset() {
  send_bytes_ = wsend_bytes_ = remain_bytes_ = 0;
  sender_.reset();
  recver_.reset();

  removeMMapObject();
  removeSendFileObject();
}

Channel::MMap *Channel::createMMapObject(int infd, size_t size) {
  if (mmap_obj_)
    return mmap_obj_;
  return mmap_obj_ = new MMap(infd, size);
}

Channel::SendFile *RawChannel::createSendFileObject(int infd, size_t size) {
  if (!sendfile_)
    return nullptr;
  if (sfile_obj_)
    return sfile_obj_;
  return sfile_obj_ = new SendFile(infd, fd_, size);
}

int SslChannel::read() {
  int n = ::SSL_read(ssl_, buffer_, kBufferSize);
  if (n > 0)
    recver_.append(buffer_, n);
  return n;
}

int SslChannel::write(bool *cflag) {
  // OpenSSL not support sendfile!
  // 1. writev(buffer)
  // 2. writev(buffer+mmap())
  *cflag = false;
  int n = -1;
  char *address = nullptr;
  // first, send all data in the buffer completely
  if (send_bytes_ < sender_.readable()) {
    address = sender_.beginRead() + send_bytes_;
    wsend_bytes_ = remain_bytes_ = sender_.readable() - send_bytes_;
    if (mmap_obj_ && mmap_obj_->getAddress())
      remain_bytes_ += mmap_obj_->getSize();
  } else {
    // there is no data to send in the send buffer
    if (mmap_obj_ && mmap_obj_->getAddress()) {
      address = mmap_obj_->getAddress() + send_bytes_ - sender_.readable();
      wsend_bytes_ = remain_bytes_ =
          mmap_obj_->getSize() - send_bytes_ + sender_.readable();
    }
  }
  n = SSL_write(ssl_, address, wsend_bytes_);
  // continue to send data
  if (n >= 0) {
    send_bytes_ += n;
    remain_bytes_ -= n;
  }
  // else n <0 : it will be sent again or an error occurs
  // all data sent successfully
  if (remain_bytes_ <= 0)
    *cflag = true;
  return n;
}

void SslChannel::close() {
  ::SSL_shutdown(ssl_);
  ::SSL_free(ssl_);
  ssl_ = nullptr;
}

int SslChannel::getError(int retcode) { return ::SSL_get_error(ssl_, retcode); }

int RawChannel::read() {
  int n = ::read(fd_, buffer_, kBufferSize);
  if (n > 0)
    recver_.append(buffer_, n);
  return n;
}

int RawChannel::write(bool *cflag) {
  // 1. writev(buffer)
  // 2. writev(buffer+mmap())
  // 3. writev(buffer)+sendfile()
  *cflag = false;
  int n = -1;
  int iovcnt = 1;
  bool sendfile_flag = false;
  struct iovec iov[2];
  ::memset(iov, 0, sizeof(iov));

  // first, send all data in the buffer completely
  if (send_bytes_ < sender_.readable()) {
    iov[0].iov_base = sender_.beginRead() + send_bytes_;
    iov[0].iov_len = sender_.readable() - send_bytes_;
    remain_bytes_ = iov[0].iov_len;

    if (mmap_obj_ && mmap_obj_->getAddress()) {
      iov[1].iov_base = mmap_obj_->getAddress();
      iov[1].iov_len = mmap_obj_->getSize();
      remain_bytes_ += iov[1].iov_len;
      iovcnt = 2;
    } else if (sfile_obj_ && sfile_obj_->getSize()) {
      remain_bytes_ += sfile_obj_->getSize();
    }
  } else {
    // there is no data to send in the send buffer
    // 1. mmap()
    // 2. sendfile()
    if (mmap_obj_ && mmap_obj_->getAddress()) {
      iov[0].iov_base =
          mmap_obj_->getAddress() + send_bytes_ - sender_.readable();
      iov[0].iov_len = mmap_obj_->getSize() - send_bytes_ + sender_.readable();
    } else if (sfile_obj_ && sfile_obj_->getSize()) {
      sendfile_flag = true;
    }
  }
  if (sendfile_flag) {
    n = sfile_obj_->sendFile();
  } else {
    n = ::writev(fd_, iov, iovcnt);
  }
  // continue to send data
  if (n >= 0) {
    send_bytes_ += n;
    remain_bytes_ -= n;
  }
  // else n <0 : it will be sent again or an error occurs
  // all data sent successfully
  if (remain_bytes_ <= 0) {
    *cflag = true;
  }
  return n;
}

void RawChannel::close() {
  ::close(fd_);
  closed_ = true;
}

int RawChannel::getError(int retcode) { return errno; }
