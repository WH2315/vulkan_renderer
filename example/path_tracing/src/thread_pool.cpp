#include "thread_pool.hpp"

ThreadPool::ThreadPool(size_t thread_count) {
    stop_ = false;
    remaining_tasks_ = 0;
    if (thread_count == 0) {
        thread_count = std::thread::hardware_concurrency();
    }
    for (size_t i = 0; i < thread_count; ++i) {
        threads_.emplace_back([this] {
            while (!stop_) {
                Task* task = dequeue();
                if (task != nullptr) {
                    task->run();
                    delete task;
                    --remaining_tasks_;
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    wait();
    stop_ = true;
    for (std::thread& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
}

void ThreadPool::enqueue(Task* task) {
    ScopedSpinLock lock(spin_lock_);
    tasks_.push(task);
    ++remaining_tasks_;
}

Task* ThreadPool::dequeue() {
    ScopedSpinLock lock(spin_lock_);
    if (tasks_.empty()) {
        return nullptr;
    }
    Task* task = tasks_.front();
    tasks_.pop();
    return task;
}

void ThreadPool::wait() const {
    while (remaining_tasks_ > 0) {
        std::this_thread::yield();
    }
}

void ThreadPool::parallelFor(size_t width, size_t height,
                             const std::function<void(size_t, size_t)>& func) {
    const size_t worker_count = threads_.empty() ? 1 : threads_.size();
    const size_t chunk_height = (height + worker_count - 1) / worker_count;
    const size_t chunk_width = width;
    for (size_t y = 0; y < height; y += chunk_height) {
        const size_t current_chunk_h = std::min(chunk_height, height - y);
        enqueue(new ParallelForTask(0, y, chunk_width, current_chunk_h, func));
    }
}