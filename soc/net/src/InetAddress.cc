#include "../include/InetAddress.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

using namespace soc::net;

InetAddress::InetAddress(uint16_t port) {
  ::memset(&sockin_, 0, sizeof(sockin_));
  sockin_.sin_port = ::htons(port);
  sockin_.sin_family = AF_INET;
  sockin_.sin_addr.s_addr = htonl(INADDR_ANY);
}

InetAddress::InetAddress(const std::string &ip, uint16_t port) {
  ::memset(&sockin_, 0, sizeof(sockin_));
  sockin_.sin_port = ::htons(port);
  sockin_.sin_family = AF_INET;
  ::inet_pton(AF_INET, ip.data(), &sockin_.sin_addr);
}

std::string InetAddress::port_s() const noexcept {
  return std::to_string(::ntohs(sockin_.sin_port));
}

std::string InetAddress::ip() const noexcept {
  char buf[64];
  ::inet_ntop(AF_INET, &sockin_.sin_addr, buf, sizeof(sockin_));
  return std::string(buf);
}