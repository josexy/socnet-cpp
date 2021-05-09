#ifndef SOC_NET_INETADDRESS_H
#define SOC_NET_INETADDRESS_H

#include <netinet/in.h>
#include <string>

namespace soc {
namespace net {

class InetAddress {
public:
  explicit InetAddress(uint16_t port = 0);
  explicit InetAddress(sockaddr_in si) : sockin_(si) {}
  explicit InetAddress(const std::string &ip, uint16_t port = 0);

  std::string ip() const noexcept;
  std::string port_s() const noexcept;
  uint16_t port() const noexcept { return ::htons(sockin_.sin_port); }
  struct sockaddr *sockaddr() const noexcept {
    return (struct sockaddr *)&sockin_;
  }
  void set_sockaddr(const sockaddr_in &sockaddr) { sockin_ = sockaddr; }
  std::string to_string() const { return ip() + ":" + port_s(); }

private:
  sockaddr_in sockin_;
};
} // namespace net
} // namespace soc

#endif
