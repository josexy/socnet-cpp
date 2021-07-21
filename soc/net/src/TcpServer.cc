#include "../include/TcpServer.h"
#include "../include/ThreadPool.h"

using namespace soc::net;
using std::placeholders::_1;

TcpServer::TcpServer()
    : quit_(false), evfd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
      svr_socket_(new ServerSocket(option::nonBlockingSocket())),
      poller_(new EPoller()), alive_timer_(new TimerQueue) {

  ::signal(SIGPIPE, SIG_IGN);
  svr_socket_->enableReuseAddr(true);
  svr_socket_->enableReusePort(true);

  option::setNonBlocking(evfd_);
  // event fd
  poller_->addEvent(evfd_, EPOLLIN | kConnectionEvent);
  // timer fd
  poller_->addEvent(alive_timer_->fd(), EPOLLIN);

  // set event callback
  poller_->setReadCallback(std::bind(&TcpServer::handleRead, this, _1));
  poller_->setWriteCallback(std::bind(&TcpServer::handleWrite, this, _1));
  poller_->setCloseCallback(std::bind(&TcpServer::handleClose, this, _1));
}

TcpServer::~TcpServer() {
  poller_->removeAndCloseEvent(evfd_);
  poller_->removeAndCloseEvent(svr_socket_->fd());
  poller_->removeAndCloseEvent(alive_timer_->fd());
  if (session_timer_)
    poller_->removeAndCloseEvent(session_timer_->fd());

  for (const auto &[fd, conn] : conns_) {
    if (!conn.disconnected())
      poller_->removeEvent(fd);
  }
}

void TcpServer::setCertificate(const std::string &cert_file,
                               const std::string &privatekey_file,
                               const std::string &password) {
  if (ssl_)
    return;
  ssl_ = std::make_unique<ServerSsl>(cert_file, privatekey_file, password);
}

void TcpServer::createSessionTimer(const TimeStamp &ts) {
  if (session_timer_ == nullptr) {
    session_timer_ = std::make_unique<TimerQueue>();
    session_timer_->runEvery(ts);
  }
  poller_->addEvent(session_timer_->fd(), EPOLLIN);
}

void TcpServer::start(const InetAddress &address) {
  svr_socket_->bind(address);
  svr_socket_->listen();
  // server socket fd
  poller_->addEvent(svr_socket_->fd(), EPOLLIN | kServerEvent);
  // alive timer
  alive_timer_->runEvery(TimeStamp::millsecond(idle_timeout_));

  // event loop
  while (!quit_) {
    if (!poller_->poll())
      continue;
  }
}

void TcpServer::quit() { wakeup(); }

void TcpServer::wakeup() {
  uint64_t one = 1;
  ::write(evfd_, &one, sizeof(one));
}

void TcpServer::handleWakeup() {
  uint64_t one = 1;
  ::read(evfd_, &one, sizeof(one));
  quit_ = true;
}

void TcpServer::handleTimeout(int fd) {
  if (alive_timer_ && alive_timer_->fd() == fd)
    alive_timer_->handleTimeout();
  else if (session_timer_ && session_timer_->fd() == fd)
    session_timer_->handleTimeout();
}

void TcpServer::handleRead(int fd) {
  if (fd == svr_socket_->fd()) {
    // Listen fd
    handleServerAccept();
  } else if (fd == evfd_) {
    // Event fd
    handleWakeup();
  } else if (alive_timer_ && (fd == alive_timer_->fd()) ||
             (session_timer_ && fd == session_timer_->fd())) {
    // Timer fd
    handleTimeout(fd);
  } else {
    // Connection fd
    handleConnectionRead(&conns_[fd]);
  }
}

void TcpServer::handleWrite(int fd) { handleConnectionWrite(&conns_[fd]); }

void TcpServer::handleClose(int fd) { handleConnectionClose(&conns_[fd]); }

void TcpServer::handleServerAccept() {
  do {
    const auto [connfd, peer] = svr_socket_->accept();
    if (connfd <= 0)
      break;

    // allocate channel
    Channel *channel = nullptr;
    if (GET_CONFIG(bool, "server", "enable_https")) {
      SslChannel *ssl_channel = ssl_->createSslChannel(connfd);
      channel = ssl_channel;
      int ret = ssl_->accept(ssl_channel);

      if (ret < 1) {
        ::ERR_print_errors_fp(stderr);
        ::close(connfd);
        delete channel;
        break;
      }
    } else {
      channel = new RawChannel(connfd);
    }
    handleConnected(connfd, channel);
  } while (0);
}

void TcpServer::handleConnected(int connfd, Channel *channel) {
  option::setNonBlocking(connfd);
  conns_[connfd].initialize(connfd);
  conns_[connfd].setChannel(channel);

  poller_->addEvent(connfd, EPOLLIN | kConnectionEvent);

  alive_timer_->add(Timer(
      connfd, TimeStamp::nowMsecond(idle_timeout_),
      std::bind(&TcpServer::handleConnectionClose, this, &conns_[connfd])));

  if (new_conn_cb_)
    new_conn_cb_(&conns_[connfd]);
}

void TcpServer::handleConnectionRead(TcpConnection *conn) {
  if (conn->disconnected() || conn->channel() == nullptr)
    return;

  alive_timer_->adjust(
      Timer(conn->fd(), TimeStamp::nowMsecond(idle_timeout_), nullptr));
  ThreadPool::instance().add(std::bind(&TcpServer::onRead, this, conn));
}

void TcpServer::handleConnectionWrite(TcpConnection *conn) {
  if (conn->disconnected() || conn->channel() == nullptr)
    return;

  alive_timer_->adjust(
      Timer(conn->fd(), TimeStamp::nowMsecond(idle_timeout_), nullptr));
  ThreadPool::instance().add(std::bind(&TcpServer::onWrite, this, conn));
}

void TcpServer::handleConnectionClose(TcpConnection *conn) {
  if (conn->disconnected() || conn->channel() == nullptr)
    return;

  poller_->removeEvent(conn->fd());
  conn->setDisconnected(true);

  // free channel
  if (conn->channel())
    conn->setChannel(nullptr);

  if (closed_cb_)
    closed_cb_(conn);
}

void TcpServer::onWrite(TcpConnection *conn) {
  const auto [n, again] = conn->writeAgain();

  if (n == 0) {
    if (conn->keepAlive()) {
      if (conn->sender()->readable() == 0) {
        // There is no data in the send buffer, so the kernel continues to
        // wait for the data to arrive
        poller_->updateEvent(conn->fd(), EPOLLIN | kConnectionEvent);
      } else {
        poller_->updateEvent(conn->fd(), EPOLLOUT | kConnectionEvent);
      }
      return;
    }
  } else if (n < 0) {
    if (again) {
      poller_->updateEvent(conn->fd(), EPOLLOUT | kConnectionEvent);
      return;
    }
  }
  handleConnectionClose(conn);
}

void TcpServer::onRead(TcpConnection *conn) {
  const auto [n, again] = conn->readAgain();
  if (n <= 0 && !again) {
    handleConnectionClose(conn);
    return;
  }
  // n < 0 && errno == EAGAIN or n < 0 && errno == SSL_ERROR_WANT_READ
  if (conn->recver()->readable() == 0)
    // There is no data in the send buffer, so the kernel continues to
    // wait for the data to arrive
    poller_->updateEvent(conn->fd(), EPOLLIN | kConnectionEvent);
  else {
    // Maybe there's a lot of data to send
    if (msg_cb_(conn)) {
      // After receiving all the data, start sending the message
      poller_->updateEvent(conn->fd(), EPOLLOUT | kConnectionEvent);
    } else {
      // Otherwise, Continue reading data
      poller_->updateEvent(conn->fd(), EPOLLIN | kConnectionEvent);
    }
  }
}
