#include "thread.h"

void testThreadSafeQueue() {
    ThreadSafeQueue<int> q1;
    atomic<bool> is_end = false;
    atomic<int> push_count = 0;
    atomic<int> pop_count = 0;

    auto producer = [&] {
        for (int i = 0; i < 1000; i++) {
            q1.push(i);
            ++push_count;
        }
    };

    auto consumer = [&] {
        int tmp = 0;
        while (!is_end || !q1.empty()) {
            while (q1.pop(tmp)) {
                ++pop_count;
            }

            if (!is_end) {
                this_thread::sleep_for(1ms);
            }
        }
    };

    vector<thread> producer_threads, consumer_threads;
    consumer_threads.emplace_back(consumer);
    for (int i = 0; i < 10; i++) {
        producer_threads.emplace_back(producer);
    }
    consumer_threads.emplace_back(consumer);
    // for (int i = 0; i < 2; i++) {
    //     consumer_threads.emplace_back(consumer);
    // }

    for (thread& t : producer_threads) {
        t.join();
    }
    is_end = true;
    for (thread& t : consumer_threads) {
        t.join();
    }
    assert(push_count == pop_count);

    ThreadSafeQueue<int> q2(2);
    q2.push(1);
    q2.push(2);
    assert(!q2.push(3));
    assert(q2.size() == 2);
}

void SpinLock::lock() {
    int failed_num = 0;
    while (is_locked.test_and_set()) {
        // 如果之前就是true则循环重试
        if (++failed_num >= 10) {
            failed_num = 0;
            this_thread::yield();
        }
    }
    // 只有由false变true才跳出循环
}

void SpinLock::unlock() {
    is_locked.clear();
}

void testSpinLock() {
    SpinLock lock;
    lock.lock();

    vector<thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back([&](int thread_idx) {
            for (int j = 0; j < 10; j++) {
                lock.lock();
                cout << thread_idx << ' ' << j << '\n';
                lock.unlock();
            }
        }, i);
    }

    lock.unlock();
    for (thread& t : threads) {
        t.join();
    }
}

ThreadPool::ThreadPool(int thread_num) {
    assert(thread_num > 0);
    threads.reserve(thread_num);
    for (int i = 0; i < thread_num; i++) {
        threads.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    if (is_shutdown) {
        return;
    }

    is_shutdown = true;
    cv.notify_all();
    for (thread& t : threads) {
        t.join();
    }
}

bool ThreadPool::isShutdown() const {
    return is_shutdown;
}

void ThreadPool::worker() {
    auto can_awake = [this] {
        return is_shutdown || !task_queue.empty();
    };

    function<void()> task;
    while (true) {
        {
            unique_lock lock(mu);
            cv.wait(lock, can_awake);
            if (task_queue.empty()) {
                if (is_shutdown) {
                    break;
                } else {
                    continue;
                }
            }

            task = move(task_queue.front());
            task_queue.pop();
        }

        task();
    }
}

void testThreadPool() {
    mutex mu;
    ThreadPool pool(4);

    // 传值
    auto add = [](int a, int b) {
        return a + b;
    };
    future<int> add_fu = pool.submit(add, 1, 2);
    assert(add_fu.get() == 3);

    // 传引用并修改
    auto modify_str = [](string& s) {
        s = "456";
    };
    string s = "123";
    future<void> modify_str_fu = pool.submit(modify_str, ref(s));
    modify_str_fu.wait();
    assert(s == "456");

    // 传右值
    auto print = [&](const string& s) {
        lock_guard lock(mu);
        cout << this_thread::get_id() << ' ' << s << '\n';
    };
    for (int i = 0; i < 100; i++) {
        pool.submit(print, to_string(i));
    }

    // 销毁时自动shutdown
    // pool.shutdown();
    cout << "shutting down" << '\n';
}
