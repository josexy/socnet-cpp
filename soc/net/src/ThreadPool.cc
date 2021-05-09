#include "../include/ThreadPool.h"

using namespace soc::net;

ThreadPool::ThreadPool(size_t max_threads) : running_(true) {
  for (size_t i = 0; i < max_threads; ++i) {
    threads_.emplace_back(&ThreadPool::run, this);
  }
}

void ThreadPool::run() {
  while (true) {
    Task task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
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

ThreadPool::~ThreadPool() {
  if (!running_)
    return;
  shutdown();
}

void ThreadPool::add(const Task &task) {
  if (!running_)
    return;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.emplace(task);
  }
  cond_.notify_one();
}

void ThreadPool::shutdown() {
  running_ = false;
  cond_.notify_all();
  for (auto &th : threads_) {
    if (th.joinable())
      th.join();
  }
}
