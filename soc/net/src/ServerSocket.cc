#include "../include/ServerSocket.h"
#include <fcntl.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace soc::net;

void option::setNonBlocking(int fd) {
  int old = ::fcntl(fd, F_GETFL, 0);
  old |= O_NONBLOCK;
  ::fcntl(fd, F_SETFL, old);
}

void option::setNoDelay(int fd) {
  int opt = 1;
  ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

int option::createNBSocket() {
  return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                  IPPROTO_TCP);
}

InetAddress option::getPeerName(int fd) {
  sockaddr_in sockaddrIn;
  socklen_t len = sizeof(sockaddrIn);
  ::getpeername(fd, (sockaddr *)&sockaddrIn, &len);
  InetAddress address(sockaddrIn);
  return address;
}

InetAddress option::getSockName(int fd) {
  sockaddr_in sockaddrIn;
  socklen_t len = sizeof(sockaddrIn);
  ::getsockname(fd, (sockaddr *)&sockaddrIn, &len);
  InetAddress address(sockaddrIn);
  return address;
}

ServerSocket::~ServerSocket() { ::close(fd_); }

void ServerSocket::bind(const InetAddress &address) {
  if (::bind(fd_, address.getSockAddr(), sizeof(sockaddr_in)) < 0)
    ::exit(-1);
}

void ServerSocket::listen() {
  if (::listen(fd_, SOMAXCONN) < 0)
    ::exit(-1);
}

std::pair<int, InetAddress> ServerSocket::accept() {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int connfd = ::accept(fd_, (sockaddr *)&addr, &len);
  InetAddress peer(addr);
  return std::make_pair(connfd, peer);
}

void ServerSocket::enableReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void ServerSocket::enableReusePort(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void ServerSocket::enableKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}
