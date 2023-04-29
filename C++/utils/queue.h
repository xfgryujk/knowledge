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

    bool full() {
        if (max_size <= 0) {
            return false;
        }
        return size() >= max_size;
    }

    size_t size() {
        // deque尺寸计算涉及好几个变量，还是加锁吧
        lock_guard lock(mu);
        return q.size();
    }
};

void testThreadSafeQueue();

template<class T>
class RingQueue {
private:
    vector<T> arr;
    // 为了区分空和满的情况，牺牲一个存储位置
    // 空的情况，head == next_tail；满的情况，(next_tail + 1) % arr_size == head
    int head_idx = 0;
    int next_tail_idx = 0;

public:
    RingQueue(size_t max_size) : arr(max_size + 1) {
        assert(max_size > 0);
    }

    bool push(const T& value) {
        if (full()) {
            return false;
        }

        arr[next_tail_idx] = value;
        if (++next_tail_idx >= arr.size()) {
            next_tail_idx = 0;
        }
        return true;
    }

    bool push(T&& value) {
        if (full()) {
            return false;
        }

        arr[next_tail_idx] = move(value);
        if (++next_tail_idx >= arr.size()) {
            next_tail_idx = 0;
        }
        return true;
    }

    bool pop(T& res) {
        if (empty()) {
            return false;
        }

        res = move(arr[head_idx]);
        if (++head_idx >= arr.size()) {
            head_idx = 0;
        }
        return true;
    }

    bool empty() const {
        return head_idx == next_tail_idx;
    }

    bool full() const {
        return (next_tail_idx + 1) % arr.size() == head_idx;
    }

    size_t size() const {
        if (head_idx <= next_tail_idx) {
            return next_tail_idx - head_idx;
        } else {
            return next_tail_idx + (arr.size() - head_idx);
        }
    }
};

void testRingQueue();
