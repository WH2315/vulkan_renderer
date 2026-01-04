#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <queue>
#include <functional>

class SpinLock {
public:
    void acquire() {
        while (flag_.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void release() { flag_.clear(std::memory_order_release); }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

class ScopedSpinLock {
public:
    explicit ScopedSpinLock(SpinLock& lock) : lock_(lock) { lock_.acquire(); }

    ~ScopedSpinLock() { lock_.release(); }

private:
    SpinLock& lock_;
};

class Task {
public:
    virtual void run() = 0;
    virtual ~Task() = default;
};

class ParallelForTask : public Task {
public:
    ParallelForTask(size_t x, size_t y, size_t chunk_width, size_t chunk_height,
                    const std::function<void(size_t, size_t)>& func)
        : x_(x),
          y_(y),
          chunk_width_(chunk_width),
          chunk_height_(chunk_height),
          func_(func) {}

    void run() override {
        for (size_t y = 0; y < chunk_height_; ++y) {
            for (size_t x = 0; x < chunk_width_; ++x) {
                func_(x_ + x, y_ + y);
            }
        }
    }

private:
    size_t x_, y_;
    size_t chunk_width_, chunk_height_;
    std::function<void(size_t, size_t)> func_;
};

class ThreadPool {
public:
    ThreadPool(size_t thread_count = 0);
    ~ThreadPool();

    void enqueue(Task* task);
    Task* dequeue();
    void wait() const;

    void parallelFor(size_t width, size_t height,
                     const std::function<void(size_t, size_t)>& func);

private:
    std::atomic_bool stop_;
    std::vector<std::thread> threads_;
    std::queue<Task*> tasks_;
    std::atomic<int> remaining_tasks_;
    SpinLock spin_lock_{};
};