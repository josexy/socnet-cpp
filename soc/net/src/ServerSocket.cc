#include "../include/ServerSocket.h"

#include <fcntl.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace soc::net;

ServerSocket::~ServerSocket() { ::close(fd_); }

void ServerSocket::bind(const InetAddress &address) {
  if (::bind(fd_, address.sockaddr(), sizeof(sockaddr_in)) < 0)
    ::exit(-1);
}

void ServerSocket::listen() {
  if (::listen(fd_, SOMAXCONN) < 0)
    ::exit(-1);
}
int ServerSocket::accept_client(InetAddress *peerAddr) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int connfd = ::accept(fd_, (sockaddr *)&addr, &len);
  if (connfd > 0) {
    peerAddr->set_sockaddr(addr);
  }
  return connfd;
}

void ServerSocket::set_reuse_address(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void ServerSocket::set_reuse_port(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void ServerSocket::set_keep_alive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void ServerSocket::set_nonblocking(int fd) {
  int old = ::fcntl(fd, F_GETFL, 0);
  old |= O_NONBLOCK;
  ::fcntl(fd, F_SETFL, old);
}

void ServerSocket::set_no_delay(int fd) {
  int opt = 1;
  ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

int ServerSocket::create_nonblock_server_fd() {
  return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                  IPPROTO_TCP);
}

InetAddress ServerSocket::peername(int fd) {
  sockaddr_in sockaddrIn;
  socklen_t len = sizeof(sockaddrIn);
  ::getpeername(fd, (sockaddr *)&sockaddrIn, &len);
  InetAddress address;
  address.set_sockaddr(sockaddrIn);
  return address;
}

InetAddress ServerSocket::sockname(int fd) {
  sockaddr_in sockaddrIn;
  socklen_t len = sizeof(sockaddrIn);
  ::getsockname(fd, (sockaddr *)&sockaddrIn, &len);
  InetAddress address;
  address.set_sockaddr(sockaddrIn);
  return address;
}
