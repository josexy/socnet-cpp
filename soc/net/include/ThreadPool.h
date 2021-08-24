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

  ~ThreadPool() {
    if (!running_)
      return;
    shutdownAll();
  }
  void add(const Task &task) {
    if (!running_)
      return;
    {
      std::lock_guard<std::mutex> lock(locker_);
      tasks_.emplace(task);
    }
    cond_.notify_one();
  }
  void shutdownAll() {
    running_ = false;
    cond_.notify_all();
    for (auto &th : threads_) {
      if (th.joinable())
        th.join();
    }
  }

private:
  ThreadPool(size_t max_threads) : running_(true) {
    for (size_t i = 0; i < max_threads; ++i) {
      threads_.emplace_back(&ThreadPool::run, this);
    }
  }
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool(const ThreadPool &&) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &&) = delete;

  void run() {
    while (true) {
      Task task;
      {
        std::unique_lock<std::mutex> lock(locker_);
        while (running_ && tasks_.empty()) {
          cond_.wait(lock);
        }
        if (!running_ && tasks_.empty())
          return;
        task = tasks_.front();
        tasks_.pop();
      }
      task();
    }
  }

private:
  std::queue<Task> tasks_;
  std::atomic<bool> running_;
  std::mutex locker_;
  std::condition_variable cond_;
  std::vector<std::thread> threads_;
};
} // namespace net
} // namespace soc

#endif
