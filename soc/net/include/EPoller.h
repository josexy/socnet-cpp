#ifndef SOC_NET_EPOLLER_H
#define SOC_NET_EPOLLER_H

#include "TimerQueue.h"
#include <atomic>
#include <functional>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
 
namespace soc {
namespace net {

namespace {
static constexpr auto kServerEvent = EPOLLRDHUP;
static constexpr auto kConnectionEvent = EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
} // namespace

class EPoller {
public:
  using ReadCallback = std::function<void(int)>;
  using WriteCallback = std::function<void(int)>;
  using CloseCallback = std::function<void(int)>;

  EPoller();
  ~EPoller();

  void addEvent(int fd, int event);
  void updateEvent(int fd, int event);
  void removeEvent(int fd);
  void removeAndCloseEvent(int fd);

  bool poll();

  void setReadCallback(const ReadCallback &callback) { read_cb_ = callback; }
  void setWriteCallback(const WriteCallback &callback) { write_cb_ = callback; }
  void setCloseCallback(const CloseCallback &callback) { close_cb_ = callback; }

private:
  int epfd_;
  std::vector<epoll_event> events_;

  ReadCallback read_cb_;
  WriteCallback write_cb_;
  CloseCallback close_cb_;
};

} // namespace net
} // namespace soc

#endif