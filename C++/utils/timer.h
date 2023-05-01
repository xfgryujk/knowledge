#pragma once

#include "common_include.h"

// 利用CRTP消除虚函数表指针、虚函数调用带来的开销
template<class T>
struct TimerHandleBase {
    void cancel() {
        static_cast<T*>(this)->cancel();
    }
};

class HeapTimerManager {
private:
    struct Timer : TimerHandleBase<Timer> {
        time_t expire = 0;
        function<void()> callback;
        // priority_queue不能随机删除元素，只能加个取消标记了
        bool is_canceled = false;

        Timer(time_t _expire, function<void()>&& _callback);

        void cancel();
    };
    using TimerPtr = shared_ptr<Timer>;
public:
    // 不想把Timer实现暴露到外部，外部只能获取到TimerHandle
    using TimerHandle = shared_ptr<TimerHandleBase<Timer>>;

private:
    struct TimerPtrCmp {
        bool operator()(const TimerPtr& a, const TimerPtr& b) const;
    };

    priority_queue<TimerPtr, vector<TimerPtr>, TimerPtrCmp> timer_queue;

public:
    TimerHandle addTimer(time_t expire, function<void()> callback);
    void update(time_t cur_time);
};

void testHeapTimerManager();

class TimeWheelTimerManager {
private:
    struct Timer : TimerHandleBase<Timer> {
        uint32_t expire = 0;
        function<void()> callback;

        // 定时器所在槽的位置，用于取消定时器
        list<shared_ptr<Timer>>* slot_ptr = nullptr;
        remove_pointer_t<decltype(slot_ptr)>::iterator slot_it;

        Timer(uint32_t _expire, function<void()>&& _callback);

        void cancel();
    };
    using TimerPtr = shared_ptr<Timer>;
public:
    // 不想把Timer实现暴露到外部，外部只能获取到TimerHandle
    using TimerHandle = shared_ptr<TimerHandleBase<Timer>>;

private:
    struct TimeWheel {
        // 索引在整数中的位置
        const uint32_t bit_offset = 0;
        // 索引的掩码
        const uint32_t bit_mask = 0;

        vector<list<TimerPtr>> slots;
        uint32_t cur_slot_idx = 0;

        TimeWheel(uint32_t _bit_offset, uint32_t bit_num);

        uint32_t getIdx(uint32_t time) const;
    };

    array<TimeWheel, 5> wheels;

public:
    TimeWheelTimerManager();

    TimerHandle addTimer(uint32_t expire, function<void()> callback);
    void update(uint32_t cur_time);
private:
    uint32_t getLastUpdateTime() const;
    void tick();
    void moveToNextSlot(int level);
};

void testTimeWheelTimerManager();
