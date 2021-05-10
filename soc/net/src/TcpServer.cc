#include "../include/TcpServer.h"

#include "../../utility/include/Logger.h"
#include "../include/ThreadPool.h"
#include <signal.h>
#include <string.h>
#include <syscall.h>
#include <unistd.h>
using namespace soc::net;

TcpServer::TcpServer()
    : epfd_(::epoll_create1(EPOLL_CLOEXEC)),
      evfd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
      socket_(ServerSocket::create_nonblock_server_fd()), quit_(false) {
  ::signal(SIGPIPE, SIG_IGN);
  socket_.set_reuse_address(true);
  socket_.set_reuse_port(true);

  ServerSocket::set_nonblocking(evfd_);
  add_fd(evfd_, EPOLLIN | kConnEvent);
}

TcpServer::~TcpServer() {
  ::close(epfd_);
  del_fd(evfd_);
  del_fd(socket_.fd());
  del_fd(tq_.fd());

  for (auto &[fd, conn] : conns_) {
    if (conn.disconnected())
      continue;
    del_fd(fd);
  }
}

#ifdef SUPPORT_SSL_LIB
void TcpServer::set_https_certificate(const char *cert_file,
                                      const char *private_key_file,
                                      const char *password) {
  if (ssl_)
    return;
  ssl_ = std::make_unique<SSLTls>(cert_file, private_key_file, password);
}
#endif

void TcpServer::start(const InetAddress &address) {
#ifdef SUPPORT_SSL_LIB
  assert(ssl_ != nullptr);
#endif

  socket_.bind(address);
  socket_.listen();
  add_fd(socket_.fd(), EPOLLIN | kServerEvent);

  add_fd(tq_.fd(), EPOLLIN);
  tq_.run_every(timestamp::millsecond(idle_timeout_));

  loop();
}

void TcpServer::quit() { wakeup(); }

void TcpServer::wakeup() {
  uint64_t one = 1;
  ::write(evfd_, &one, sizeof(one));
}

void TcpServer::loop() {
  events_.resize(1024);
  while (!quit_) {
    int n = ::epoll_wait(epfd_, events_.data(), events_.size(), -1);
    if (n < 0)
      continue;

    if (n >= (int)events_.size())
      events_.resize(n * 2);

    for (int i = 0; i < n; ++i) {
      int fd = events_[i].data.fd;
      int revents = events_[i].events;
      if (fd == socket_.fd()) { // Listenfd
        handle_server_listen();
      } else if (evfd_ == fd && revents & EPOLLIN) { // Eventfd
        handle_wakeup();
      } else if (tq_.fd() == fd && revents & EPOLLIN) { // Timerfd
        handle_timeout();
      } else if (revents & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) { // Connfd
        handle_conn_close(&conns_[fd]);
      } else if (revents & EPOLLIN) {
        handle_conn_read(&conns_[fd]);
      } else if (revents & EPOLLOUT) {
        handle_conn_write(&conns_[fd]);
      }
    }
  }
}

void TcpServer::handle_wakeup() {
  uint64_t one = 1;
  ::read(evfd_, &one, sizeof(one));
  quit_ = true;
}

void TcpServer::handle_timeout() { tq_.handle_timeout(); }

void TcpServer::handle_server_listen() {
  do {
    InetAddress peer;
    int connfd = socket_.accept_client(&peer);
    if (connfd <= 0)
      break;

#ifdef SUPPORT_SSL_LIB
    SSL *ssl = ssl_->ssl_create_conn(connfd);
    int ret = ssl_->accept_conn(ssl);
    if (ret < 1) {
      ::ERR_print_errors_fp(stderr);
      ::SSL_shutdown(ssl);
      ::SSL_free(ssl);
      ::close(connfd);
      break;
    }
    handle_connected_conn(connfd, ssl);
#else
    handle_connected_conn(connfd);
#endif
  } while (0);
}

#ifdef SUPPORT_SSL_LIB
void TcpServer::handle_connected_conn(int connfd, SSL *ssl) {
  handle_connected_conn(connfd);
  conns_[connfd].set_ssl(ssl);
}
#endif

void TcpServer::handle_connected_conn(int connfd) {
  ServerSocket::set_nonblocking(connfd);
  conns_[connfd].initialize(connfd);
  add_fd(connfd, EPOLLIN | kConnEvent);

  tq_.add(connfd, timestamp::now_msecond(idle_timeout_),
          std::bind(&TcpServer::handle_conn_close, this, &conns_[connfd]));

  if (new_conn_cb_)
    new_conn_cb_(&conns_[connfd]);
}

void TcpServer::handle_conn_close(TcpConnectionPtr conn) {
  if (conn->disconnected())
    return;

  conn->disconnected(true);

#ifdef SUPPORT_SSL_LIB
  ::SSL_shutdown(conn->ssl());
  ::SSL_free(conn->ssl());
#endif
  del_fd(conn->fd());
  if (close_cb_)
    close_cb_(conn);
}

void TcpServer::handle_conn_read(TcpConnectionPtr conn) {
  if (conn->disconnected())
    return;

  tq_.adjust(conn->fd(), timestamp::now_msecond(idle_timeout_));
  ThreadPool::instance().add(std::bind(&TcpServer::on_read, this, conn));
}

void TcpServer::handle_conn_write(TcpConnectionPtr conn) {
  if (conn->disconnected())
    return;

  tq_.adjust(conn->fd(), timestamp::now_msecond(idle_timeout_));
  ThreadPool::instance().add(std::bind(&TcpServer::on_write, this, conn));
}

void TcpServer::on_write(TcpConnection *conn) {
  int err;
  int n = conn->write(&err);

  if (n == 0) {
    if (conn->keep_alive()) {
      if (conn->sender()->readable() == 0) {
        // There is no data in the send buffer, so the kernel continues to
        // wait for the data to arrive
        mod_fd(conn->fd(), EPOLLIN | kConnEvent);
      } else {
        mod_fd(conn->fd(), EPOLLOUT | kConnEvent);
      }
      return;
    }
  } else if (n < 0) {
#ifdef SUPPORT_SSL_LIB
    if (err == SSL_ERROR_WANT_WRITE) {
#else
    if (err == EAGAIN) {
#endif
      // Continue sending messages
      mod_fd(conn->fd(), EPOLLOUT | kConnEvent);
      return;
    }
  }
  handle_conn_close(conn);
}

void TcpServer::on_read(TcpConnection *conn) {
  int err;
  int n = conn->read(&err);

#ifdef SUPPORT_SSL_LIB
  if (n <= 0 && err != SSL_ERROR_WANT_READ) {
#else
  if (n <= 0 && err != EAGAIN) {
#endif
    handle_conn_close(conn);
    return;
  }
  // n < 0 && errno == EAGAIN or n < 0 && errno == SSL_ERROR_WANT_READ
  if (conn->recver()->readable() == 0)
    // There is no data in the send buffer, so the kernel continues to
    // wait for the data to arrive
    mod_fd(conn->fd(), EPOLLIN | kConnEvent);
  else {
    // Maybe there's a lot of data to send
    if (msg_cb_(conn)) {
      // After receiving all the data, start sending the message
      mod_fd(conn->fd(), EPOLLOUT | kConnEvent);
    } else {
      // Otherwise, Continue reading data
      mod_fd(conn->fd(), EPOLLIN | kConnEvent);
    }
  }
}

void TcpServer::add_fd(int fd, int event) {
  struct epoll_event ee;
  ::memset(&ee, 0, sizeof(ee));
  ee.data.fd = fd;
  ee.events = event;
  ::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ee);
}

void TcpServer::mod_fd(int fd, int event) {
  struct epoll_event ee;
  ::memset(&ee, 0, sizeof(ee));
  ee.data.fd = fd;
  ee.events = event;
  ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee);
}

void TcpServer::del_fd(int fd) {
  ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
  ::close(fd);
}
