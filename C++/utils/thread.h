#pragma once

#include "common_include.h"

class SpinLock {
private:
    atomic_flag is_locked = ATOMIC_FLAG_INIT;

public:
    void lock();
    void unlock();
};

void testSpinLock();

class SharedMutex {
private:
    // 已经加锁的读者数
    int reader_num = 0;
    // 读者需要加的锁，用来保证操作reader_num和writer_mu的原子性
    mutex reader_mu;
    // 写者和第一个读者需要加的锁
    mutex writer_mu;
    // 读写都要加的锁，用来实现公平竞争。没有这个锁也行，但是会偏向读者，写者可能饥饿
    mutex common_mu;

public:
    void lock();
    void unlock();
    void lock_shared();
    void unlock_shared();
};

void testSharedMutex();

// 条件变量实现的信号量，C++标准库没有信号量时可以使用
class Semaphore {
private:
    atomic<int> counter = 0;
    mutex mu;
    condition_variable cv;

public:
    Semaphore(int _counter);

    void acquire(size_t dec_num = 1);
    void release(size_t add_num = 1);
};

void testSemaphore();

class ThreadPool {
private:
    vector<thread> threads;

    atomic<bool> is_shutdown = false;
    queue<function<void()>> task_queue;
    mutex mu;
    condition_variable cv;

public:
    ThreadPool(int thread_num);
    ~ThreadPool();

    void shutdown();
    bool isShutdown() const;
private:
    void worker();

public:
    template<class Callable, class... Args>
    auto submit(Callable&& callable, Args&&... args) {
        static_assert(is_invocable_v<Callable, Args...>);
        using Result = invoke_result_t<Callable, Args...>;

        // function要求底层必须能拷贝，这里只能用指针
        auto task_ptr = make_shared<packaged_task<Result(Args...)>>(forward<Callable>(callable));
        // 为了保证调用时参数还没被销毁，这里用值捕获
        auto callback = [=]() mutable {
            invoke(*task_ptr, forward<Args>(args)...);
        };

        {
            lock_guard lock(mu);
            assert(!is_shutdown);
            task_queue.emplace(move(callback));
        }
        cv.notify_one();

        return task_ptr->get_future();
    }
};

void testThreadPool();
