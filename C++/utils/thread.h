#pragma once

#include "common_include.h"

template<class T>
class ThreadSafeQueue {
private:
    queue<T> q;
    mutex mu;

    // 0表示不限制
    const size_t max_size = 0;

public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue(size_t _max_size) : max_size(_max_size) {
    }

    bool push(const T& value) {
        lock_guard lock(mu);
        if (max_size > 0 && q.size() >= max_size) {
            return false;
        }
        q.emplace(value);
        return true;
    }

    bool push(T&& value) {
        lock_guard lock(mu);
        if (max_size > 0 && q.size() >= max_size) {
            return false;
        }
        q.emplace(move(value));
        return true;
    }

    bool pop(T& res) {
        lock_guard lock(mu);
        if (q.empty()) {
            return false;
        }
        res = move(q.front());
        q.pop();
        return true;
    }

    bool empty() {
        lock_guard lock(mu);
        return q.empty();
    }

    size_t size() {
        // deque尺寸计算涉及好几个变量，还是加锁吧
        lock_guard lock(mu);
        return q.size();
    }
};

void testThreadSafeQueue();

class SpinLock {
private:
    atomic_flag is_locked = ATOMIC_FLAG_INIT;

public:
    void lock();
    void unlock();
};

void testSpinLock();

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
        auto task_ptr = new packaged_task<Result(Args...)>(forward<Callable>(callable));
        future<Result> fu = task_ptr->get_future();

        {
            lock_guard lock(mu);
            assert(!is_shutdown);
            task_queue.emplace(
                // 为了保证调用时参数还没被销毁，这里用值捕获
                [=]() mutable {
                    invoke(*task_ptr, forward<Args>(args)...);
                    delete task_ptr;
                }
            );
        }
        cv.notify_one();

        return fu;
    }
};

void testThreadPool();
