#ifndef SOC_NET_TCPSERVER_H
#define SOC_NET_TCPSERVER_H

#include "../../utility/include/AppConfig.h"
#include "EPoller.h"
#include "ServerSsl.h"
#include "TcpConnection.h"
#include <signal.h>

namespace soc {
namespace net {

class TcpServer {
public:
  using NewConnectionCallback = std::function<void(TcpConnection *)>;
  using MessageCallback = std::function<bool(TcpConnection *)>;
  using ClosedConnectionCallback = std::function<void(TcpConnection *)>;

  TcpServer();
  ~TcpServer();

  InetAddress getInetAddress() {
    return option::getSockName(svr_socket_->getFd());
  }

  void start(const InetAddress &address);
  void quit();
  void wakeUp();

  void setIdleTime(int millsecond) { idle_timeout_ = millsecond; }

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    new_conn_cb_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { msg_cb_ = cb; }
  void setClosedConnectionCallback(const ClosedConnectionCallback &cb) {
    closed_cb_ = cb;
  }

  void setCertificate(const std::string &cert_file,
                      const std::string &privatekey_file,
                      const std::string &password);

  void createSessionTimer(const TimeStamp &);

  TimerQueue *getClientTimer() const noexcept { return alive_timer_.get(); }
  TimerQueue *getSessionTimer() const noexcept { return session_timer_.get(); }
  EPoller *getEPoller() const noexcept { return poller_.get(); }

private:
  void handleServerAccept();
  void handleRead(int);
  void handleWrite(int);
  void handleClose(int);
  void handleTimeout(int);
  void handleWakeup();

  void handleConnected(int, Channel *);
  void handleConnectionRead(TcpConnection *);
  void handleConnectionWrite(TcpConnection *);
  void handleConnectionClose(TcpConnection *);

  void onWrite(TcpConnection *);
  void onRead(TcpConnection *);

private:
  int evfd_;
  int idle_timeout_;
  std::atomic<bool> quit_;
  bool sendfile_;

  std::unique_ptr<ServerSocket> svr_socket_;
  std::unique_ptr<EPoller> poller_;
  std::unique_ptr<TimerQueue> alive_timer_;
  std::unique_ptr<TimerQueue> session_timer_;
  std::unique_ptr<ServerSsl> ssl_;

  std::unordered_map<int, TcpConnection> conns_;

  NewConnectionCallback new_conn_cb_;
  MessageCallback msg_cb_;
  ClosedConnectionCallback closed_cb_;
};
} // namespace net

} // namespace soc

#endif
