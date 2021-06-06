#ifndef SOC_NET_TCPSERVER_H
#define SOC_NET_TCPSERVER_H

#include "../../utility/include/AppConfig.h"
#include "TcpConnection.h"
#include "TimerQueue.h"
#include <atomic>
#include <memory>
#include <sys/epoll.h>
#include <sys/eventfd.h>
namespace soc {
namespace net {

namespace {
constexpr static auto kServerEvent = EPOLLRDHUP;
constexpr static auto kConnEvent = EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
} // namespace

class TimerQueue;
class TcpServer {
public:
  using ConnCb = std::function<void(TcpConnectionPtr)>;
  using MsgCb = std::function<bool(TcpConnectionPtr)>;
  using CloseCb = std::function<void(TcpConnectionPtr)>;

  TcpServer();
  ~TcpServer();

  InetAddress address() const { return ServerSocket::sockname(socket_.fd()); }

  void start(const InetAddress &address);
  void quit();
  void wakeup();

  void set_idle_timeout(int millsecond) { idle_timeout_ = millsecond; }
  void set_new_conn_cb(const ConnCb &cb) { new_conn_cb_ = std::move(cb); }
  void set_msg_cb(const MsgCb &cb) { msg_cb_ = std::move(cb); }
  void set_close_conn_cb(const CloseCb &cb) { close_cb_ = std::move(cb); }

  void set_https_certificate(const char *cert_file,
                             const char *private_key_file,
                             const char *password = nullptr);

private:
  void loop();

  void handle_server_listen();
  void handle_conn_read(TcpConnectionPtr conn);
  void handle_conn_write(TcpConnectionPtr conn);
  void handle_conn_close(TcpConnectionPtr conn);
  void handle_wakeup();
  void handle_timeout();

  void handle_connected_conn(int connfd, SSL *ssl);
  void handle_connected_conn(int connfd);

  void on_write(TcpConnectionPtr conn);
  void on_read(TcpConnectionPtr conn);

  void add_fd(int fd, int event);
  void mod_fd(int fd, int event);
  void del_fd(int fd);

private:
  int epfd_;
  int evfd_;
  int idle_timeout_;
  ServerSocket socket_;
  std::atomic<bool> quit_;

  std::unique_ptr<SSLTls> ssl_;
  std::vector<epoll_event> events_;
  std::unordered_map<int, TcpConnection> conns_;
  TimerQueue tq_;

  ConnCb new_conn_cb_;
  MsgCb msg_cb_;
  CloseCb close_cb_;
};
} // namespace net

} // namespace soc

#endif
