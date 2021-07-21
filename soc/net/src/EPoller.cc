#include "../include/EPoller.h"
using namespace soc::net;

EPoller::EPoller() : epfd_(::epoll_create1(EPOLL_CLOEXEC)) {}

EPoller::~EPoller() {}

void EPoller::addEvent(int fd, int event) {
  struct epoll_event ee;
  ::memset(&ee, 0, sizeof(ee));
  ee.data.fd = fd;
  ee.events = event;
  ::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ee);
}

void EPoller::updateEvent(int fd, int event) {
  struct epoll_event ee;
  ::memset(&ee, 0, sizeof(ee));
  ee.data.fd = fd;
  ee.events = event;
  ::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ee);
}

void EPoller::removeEvent(int fd) {
  ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
}

void EPoller::removeAndCloseEvent(int fd) {
  removeEvent(fd);
  ::close(fd);
}

bool EPoller::poll() {
  events_.resize(1024);
  int n = ::epoll_wait(epfd_, events_.data(), events_.size(), -1);
  if (n < 0)
    return false;

  if (n >= (int)events_.size())
    events_.resize(n * 2);

  for (int i = 0; i < n; ++i) {
    int fd = events_[i].data.fd;
    int revents = events_[i].events;

    if (revents & EPOLLIN) {
      if (read_cb_)
        read_cb_(fd);
    } else if (revents & EPOLLOUT) {
      if (write_cb_)
        write_cb_(fd);
    } else if (revents & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) {
      if (close_cb_)
        close_cb_(fd);
    }
  }
  return true;
}
