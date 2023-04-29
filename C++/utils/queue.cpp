#include "queue.h"

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

void testRingQueue() {
    RingQueue<int> q(2);

    for (int i = 0; i < 10; i++) {
        q.push(i);
        assert(q.size() == 1);
        assert(!q.empty());
        assert(!q.full());

        q.push(i + 1);
        assert(q.size() == 2);
        assert(!q.empty());
        assert(q.full());

        assert(!q.push(i + 2));

        int tmp = 0;
        q.pop(tmp);
        assert(tmp == i);
        assert(q.size() == 1);
        assert(!q.empty());
        assert(!q.full());

        q.pop(tmp);
        assert(tmp == i + 1);
        assert(q.size() == 0);
        assert(q.empty());
        assert(!q.full());

        assert(!q.pop(tmp));
    }
}
