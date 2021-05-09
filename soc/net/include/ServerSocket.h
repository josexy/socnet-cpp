#ifndef SOC_NET_SERVERSOCKET_H
#define SOC_NET_SERVERSOCKET_H

#include "InetAddress.h"

namespace soc {
namespace net {
class ServerSocket {
public:
  explicit ServerSocket(int sockfd) : fd_(sockfd) {}
  ~ServerSocket();

  int fd() const noexcept { return fd_; }
  void bind(const InetAddress &address);
  void listen();
  void set_reuse_address(bool on);
  void set_reuse_port(bool on);
  void set_keep_alive(bool on);
  int accept_client(InetAddress *peerAddr);

  static int create_nonblock_server_fd();

  static void set_nonblocking(int fd);
  static void set_no_delay(int fd);
  static InetAddress peername(int fd);
  static InetAddress sockname(int fd);

private:
  int fd_;
};

} // namespace net

} // namespace soc

#endif
