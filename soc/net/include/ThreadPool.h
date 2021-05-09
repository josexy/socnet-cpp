#ifndef SOC_NET_THREADPOOL_H
#define SOC_NET_THREADPOOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace soc {
namespace net {

class ThreadPool {
public:
  using Task = std::function<void()>;

  static ThreadPool &instance() {
    static ThreadPool th(std::thread::hardware_concurrency());
    return th;
  }

  ~ThreadPool();
  void add(const Task &task);
  void shutdown();

private:
  ThreadPool(size_t max_threads);
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(const ThreadPool &&) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &&) = delete;

  void run();

private:
  std::queue<Task> tasks_;
  std::atomic<bool> running_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::vector<std::thread> threads_;
};

} // namespace net
} // namespace soc

#endif
