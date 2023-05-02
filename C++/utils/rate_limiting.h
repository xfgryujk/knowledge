#pragma once

#include "common_include.h"

// 固定窗口，缺点是当请求集中在边界两侧时，有一段时间请求数会超出限制
class FixedWindowRateLimiter {
private:
    const time_t window_size = 0;
    const int max_request_num = 0;

    time_t start_time = 0;
    int request_num = 0;

public:
    FixedWindowRateLimiter(time_t _window_size, int _max_request_num);

    bool addRequest(time_t cur_time);
};

void testFixedWindowRateLimiter();

// 滑动窗口，缓解了临界问题，但当两端窗口的请求集中在靠近中间的边界时，还是会有一段时间超出限制
class SlidingWindowRateLimiter {
private:
    struct Window {
        const time_t start_time = 0;
        int request_num = 1;

        Window(time_t _start_time);
    };

    const time_t window_size = 0;
    const int window_num = 0;
    const int max_request_num = 0;

    queue<Window> window_queue;
    int request_num_sum = 0;

public:
    SlidingWindowRateLimiter(time_t _window_size, int _window_num, int _max_request_num);

    bool addRequest(time_t cur_time);
};

void testSlidingWindowRateLimiter();

// 相当于窗口尺寸为1的滑动窗口，缺点是浪费内存
class PreciseRateLimiter {
private:
    const time_t window_size = 0;
    const int max_request_num = 0;

    queue<time_t> request_time_queue;

public:
    PreciseRateLimiter(time_t _window_size, int _max_request_num);

    bool addRequest(time_t cur_time);
};

void testPreciseRateLimiter();

// 令牌桶，定期向桶里添加令牌，只有拿到令牌才能处理请求
// 类似于网游中的体力恢复算法
class TokenBucketRateLimiter {
private:
    // 每秒生成的token数
    const double tokens_per_sec = 0;
    const size_t max_token_num = 0;

    // 上次更新后有多少token
    size_t stored_token_num = 0;
    double last_update_time = 0;

public:
    TokenBucketRateLimiter(double _tokens_per_sec, size_t _max_token_num);

    bool addRequest(time_t cur_time);
};

void testTokenBucketRateLimiter();

// 漏桶就是consumer定期从消息队列取消息处理，和这里的接口不同，就不演示了
