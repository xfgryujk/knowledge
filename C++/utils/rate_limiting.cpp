#include "rate_limiting.h"

FixedWindowRateLimiter::FixedWindowRateLimiter(time_t _window_size, int _max_request_num) :
    window_size(_window_size),
    max_request_num(_max_request_num) {
    assert(window_size > 0);
}

bool FixedWindowRateLimiter::addRequest(time_t cur_time) {
    if (cur_time >= start_time + window_size) {
        start_time = cur_time;
        request_num = 0;
    }
    if (request_num >= max_request_num) {
        return false;
    }

    request_num++;
    return true;
}

void testFixedWindowRateLimiter() {
    FixedWindowRateLimiter limiter(10, 10);
    constexpr time_t START_TIME = 1000;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 2; j++) {
            assert(limiter.addRequest(START_TIME + i));
        }
    }
    assert(!limiter.addRequest(START_TIME + 5));
    assert(!limiter.addRequest(START_TIME + 9));

    assert(limiter.addRequest(START_TIME + 10));
}

SlidingWindowRateLimiter::Window::Window(time_t _start_time) :
    start_time(_start_time) {
}

SlidingWindowRateLimiter::SlidingWindowRateLimiter(time_t _window_size, int _window_num, int _max_request_num) :
    window_size(_window_size),
    window_num(_window_num),
    max_request_num(_max_request_num) {
    assert(window_size > 0);
    assert(window_num > 0);
}

bool SlidingWindowRateLimiter::addRequest(time_t cur_time) {
    time_t min_start_time = cur_time - window_size * window_num;
    while (!window_queue.empty() && window_queue.front().start_time <= min_start_time) {
        request_num_sum -= window_queue.front().request_num;
        window_queue.pop();
    }
    if (request_num_sum >= max_request_num) {
        return false;
    }

    request_num_sum++;
    if (!window_queue.empty() && cur_time < window_queue.back().start_time + window_size) {
        // 当前时间在最后一个窗口内
        window_queue.back().request_num++;
    } else {
        window_queue.emplace(cur_time);
    }
    return true;
}

void testSlidingWindowRateLimiter() {
    SlidingWindowRateLimiter limiter(2, 5, 10);
    constexpr time_t START_TIME = 1000;
    for (int i = 0; i < 10; i++) {
        assert(limiter.addRequest(START_TIME + i));
    }
    assert(!limiter.addRequest(START_TIME + 9));

    // 删除开头的窗口后能再发2个请求
    assert(limiter.addRequest(START_TIME + 10));
    assert(limiter.addRequest(START_TIME + 10));
    assert(!limiter.addRequest(START_TIME + 10));
    assert(!limiter.addRequest(START_TIME + 11));
    assert(limiter.addRequest(START_TIME + 12));

    // 过了很长时间，窗口清空了
    constexpr time_t START_TIME_2 = START_TIME + 1000;
    for (int i = 0; i < 10; i++) {
        assert(limiter.addRequest(START_TIME_2));
    }
    assert(!limiter.addRequest(START_TIME_2 + 9));

    for (int i = 0; i < 10; i++) {
        assert(limiter.addRequest(START_TIME_2 + 10));
    }
    assert(!limiter.addRequest(START_TIME_2 + 10));
}

PreciseRateLimiter::PreciseRateLimiter(time_t _window_size, int _max_request_num) :
    window_size(_window_size),
    max_request_num(_max_request_num) {
    assert(window_size > 0);
}

bool PreciseRateLimiter::addRequest(time_t cur_time) {
    time_t min_time = cur_time - window_size;
    while (!request_time_queue.empty() && request_time_queue.front() <= min_time) {
        request_time_queue.pop();
    }
    if (request_time_queue.size() >= max_request_num) {
        return false;
    }

    request_time_queue.emplace(cur_time);
    return true;
}

void testPreciseRateLimiter() {
    PreciseRateLimiter limiter(10, 10);
    constexpr time_t START_TIME = 1000;
    for (int i = 0; i < 10; i++) {
        assert(limiter.addRequest(START_TIME + i));
    }
    assert(!limiter.addRequest(START_TIME + 9));

    assert(limiter.addRequest(START_TIME + 10));
    assert(!limiter.addRequest(START_TIME + 10));

    // 过了很长时间，窗口清空了
    constexpr time_t START_TIME_2 = START_TIME + 1000;
    for (int i = 0; i < 10; i++) {
        assert(limiter.addRequest(START_TIME_2));
    }
    assert(!limiter.addRequest(START_TIME_2 + 9));
}

TokenBucketRateLimiter::TokenBucketRateLimiter(double _tokens_per_sec, size_t _max_token_num) :
    tokens_per_sec(_tokens_per_sec),
    max_token_num(_max_token_num),
    stored_token_num(max_token_num) {
    assert(tokens_per_sec > 0);
}

bool TokenBucketRateLimiter::addRequest(time_t cur_time) {
    double cur_time_float = max((double)cur_time, last_update_time);

    size_t add_token_num = size_t((cur_time_float - last_update_time) * tokens_per_sec);
    // 减法防止溢出
    if (add_token_num < max_token_num - stored_token_num) {
        stored_token_num += add_token_num;
        last_update_time += (double)add_token_num / tokens_per_sec;
    } else {
        // 桶已经满了就不再生成token了，这次请求减少token后再开始生成
        stored_token_num = max_token_num;
        last_update_time = cur_time_float;
    }
    if (stored_token_num <= 0) {
        return false;
    }

    stored_token_num--;
    return true;
}

void testTokenBucketRateLimiter() {
    TokenBucketRateLimiter limiter(3., 7);
    constexpr time_t START_TIME = 1680000000;
    // 第1秒用掉5个剩2个，第2秒生成3个用掉5个剩0个
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 5; j++) {
            assert(limiter.addRequest(START_TIME + i));
        }
    }
    assert(!limiter.addRequest(START_TIME + 1));
    assert(limiter.addRequest(START_TIME + 2));

    // 过了很长时间
    constexpr time_t START_TIME_2 = START_TIME + 1000;
    for (int i = 0; i < 7; i++) {
        assert(limiter.addRequest(START_TIME_2));
    }
    assert(!limiter.addRequest(START_TIME_2));

    // 过1秒生成3个token
    for (int i = 0; i < 3; i++) {
        assert(limiter.addRequest(START_TIME_2 + 1));
    }
    assert(!limiter.addRequest(START_TIME_2 + 1));

    // 传一个过去的时间
    assert(!limiter.addRequest(START_TIME_2));
}
