#include "thread.h"

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

void SharedMutex::lock() {
    // 读写都要先抢公共锁，实现公平竞争
    common_mu.lock();
    writer_mu.lock();
}

void SharedMutex::unlock() {
    writer_mu.unlock();
    common_mu.unlock();
}

void SharedMutex::lock_shared() {
    // 读写都要先抢公共锁，实现公平竞争
    lock_guard common_lock(common_mu);
    // 第一个读者加写锁
    lock_guard reader_lock(reader_mu);
    if (reader_num++ == 0) {
        // 如果有写者已经加锁，这里会阻塞，并且其他读者会阻塞在reader_lock
        writer_mu.lock();
    }
}

void SharedMutex::unlock_shared() {
    // 最后一个读者解除写锁
    lock_guard reader_lock(reader_mu);
    if (--reader_num == 0) {
        writer_mu.unlock();
    }
}

void testSharedMutex() {
    SharedMutex mu;
    vector<int> arr;

    auto reader = [&](int thread_idx) {
        for (int i = 0; i < 10000; i++) {
            int tmp = 0;
            mu.lock_shared();
            cout << "reader " << thread_idx << ' ' << i << '\n';
            int num = min((int)arr.size(), 100);
            for (int j = 0; j < num; j++) {
                tmp += arr[j];
            }
            mu.unlock_shared();

            this_thread::sleep_for(chrono::milliseconds(rand() % 5));
        }
    };

    auto writer = [&](int thread_idx) {
        for (int i = 0; i < 10000; i++) {
            mu.lock();
            cout << "writer " << thread_idx << ' ' << i << '\n';
            // this_thread::sleep_for(200ms);
            arr.push_back(i);
            mu.unlock();

            this_thread::sleep_for(chrono::milliseconds(rand() % 100));
        }
    };

    vector<thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back(writer, i);
        threads.emplace_back(reader, i);
    }
    for (thread& t : threads) {
        t.join();
    }

    assert(arr.size() == 50000);
}

Semaphore::Semaphore(int _counter) : counter(_counter) {
}

void Semaphore::acquire(size_t dec_num) {
    unique_lock lock(mu);
    cv.wait(lock, [this, dec_num] { return counter >= dec_num; });
    counter -= dec_num;
}

void Semaphore::release(size_t add_num) {
    if (add_num == 0) {
        return;
    }
    counter += add_num;
    cv.notify_all();
}

void testSemaphore() {
    Semaphore sem(2);

    vector<thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&](int thread_idx) {
            sem.acquire();
            cout << thread_idx << '\n';
        }, i);
    }

    this_thread::sleep_for(1s);
    sem.release();
    this_thread::sleep_for(1s);
    sem.release(2);

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

    // 调用成员函数
    future<size_t> size_fu = pool.submit(&string::size, &s);
    assert(size_fu.get() == 3);

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
