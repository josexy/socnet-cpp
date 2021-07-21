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
  uint16_t port() const noexcept { return ::htons(sockin_.sin_port); }
  sockaddr *saddr() const noexcept { return (struct sockaddr *)&sockin_; }
  std::string toString() const { return ip() + ":" + std::to_string(port()); }

private:
  sockaddr_in sockin_;
};
} // namespace net
} // namespace soc

#endif
