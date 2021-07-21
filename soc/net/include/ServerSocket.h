#ifndef SOC_NET_SERVERSOCKET_H
#define SOC_NET_SERVERSOCKET_H

#include "InetAddress.h"

namespace soc {
namespace net {

namespace option {
int nonBlockingSocket();
void setNonBlocking(int fd);
void setNoDelay(int fd);
InetAddress peername(int fd);
InetAddress sockname(int fd);
} // namespace option

class ServerSocket {
public:
  explicit ServerSocket(int sockfd) : fd_(sockfd) {}
  ~ServerSocket();

  int fd() const noexcept { return fd_; }
  void bind(const InetAddress &address);
  void listen();
  std::pair<int, InetAddress> accept();

  void enableReuseAddr(bool on);
  void enableReusePort(bool on);
  void enableKeepAlive(bool on);

private:
  int fd_;
};

} // namespace net

} // namespace soc

#endif
