#include "timer.h"

HeapTimerManager::Timer::Timer(time_t _expire, function<void()>&& _callback) :
    expire(_expire),
    callback(move(_callback)) {
}

void HeapTimerManager::Timer::cancel() {
    is_canceled = true;
}

bool HeapTimerManager::TimerPtrCmp::operator()(const TimerPtr& a, const TimerPtr& b) const {
    return a->expire > b->expire;
}

HeapTimerManager::TimerHandle HeapTimerManager::addTimer(time_t expire, function<void()> callback) {
    TimerPtr timer_ptr = make_shared<Timer>(expire, move(callback));
    timer_queue.emplace(timer_ptr);
    return static_pointer_cast<TimerHandleBase<Timer>>(timer_ptr);
}

void HeapTimerManager::update(time_t cur_time) {
    while (!timer_queue.empty() && timer_queue.top()->expire <= cur_time) {
        TimerPtr timer_ptr = move(timer_queue.top());
        timer_queue.pop();
        if (timer_ptr->is_canceled) {
            continue;
        }

        timer_ptr->callback();
    }
}

void testHeapTimerManager() {
    auto make_print_callback = [](int i) {
        return [=] {
            cout << i << '\n';
        };
    };

    HeapTimerManager timer_mgr;
    timer_mgr.addTimer(1, make_print_callback(1));
    timer_mgr.addTimer(1, make_print_callback(2));
    timer_mgr.addTimer(10, make_print_callback(3));
    timer_mgr.addTimer(11, make_print_callback(4))->cancel();
    timer_mgr.addTimer(12, make_print_callback(5));
    timer_mgr.addTimer(12, make_print_callback(6));
    timer_mgr.addTimer(13, make_print_callback(7));

    for (time_t cur_time : {1, 2, 9, 10, 11, 13}) {
        cout << "cur_time = " << cur_time << '\n';
        timer_mgr.update(cur_time);
    }
}

TimeWheelTimerManager::Timer::Timer(uint32_t _expire, function<void()>&& _callback) :
    expire(_expire),
    callback(move(_callback)) {
}

void TimeWheelTimerManager::Timer::cancel() {
    if (slot_ptr == nullptr) {
        return;
    }
    slot_ptr->erase(slot_it);
    slot_ptr = nullptr;
    slot_it = decltype(slot_it)();
}

TimeWheelTimerManager::TimeWheel::TimeWheel(uint32_t _bit_offset, uint32_t bit_num) :
    bit_offset(_bit_offset),
    bit_mask((1 << bit_num) - 1),
    slots(bit_mask + 1) {
}

uint32_t TimeWheelTimerManager::TimeWheel::getIdx(uint32_t time) const {
    return (time >> bit_offset) & bit_mask;
}

TimeWheelTimerManager::TimeWheelTimerManager() :
    wheels{
        TimeWheel{0, 6},
        TimeWheel{6, 6},
        TimeWheel{12, 6},
        TimeWheel{18, 6},
        TimeWheel{24, 8}
    } {
}

TimeWheelTimerManager::TimerHandle TimeWheelTimerManager::addTimer(uint32_t expire, function<void()> callback) {
    TimerPtr timer_ptr = make_shared<Timer>(expire, move(callback));

    for (int level = wheels.size() - 1; level >= 0; level--) {
        TimeWheel& wheel = wheels[level];
        uint32_t idx = wheel.getIdx(expire);
        if (idx <= wheel.cur_slot_idx) {
            if (level > 0) {
                // 如果不是最低级，因为当前槽已经加载到更低级的轮了，只能放到更低级的轮上
                continue;
            }
            // 已经是最低级了，只能放到即将执行的槽
            idx = wheel.cur_slot_idx;
        }

        // 找到应该放的槽了
        list<TimerPtr>& slot = wheel.slots[idx];
        slot.emplace_back(timer_ptr);
        timer_ptr->slot_ptr = &slot;
        timer_ptr->slot_it = --slot.end();
        break;
    }

    return static_pointer_cast<TimerHandleBase<Timer>>(timer_ptr);
}

void TimeWheelTimerManager::update(uint32_t cur_time) {
    uint32_t last_update_time = getLastUpdateTime();
    uint32_t move_num = (cur_time <= last_update_time) ? 0 : (cur_time - last_update_time);
    for (uint32_t i = 0; i < move_num; i++) {
        tick();
        moveToNextSlot(0);
    }
    tick();
}

uint32_t TimeWheelTimerManager::getLastUpdateTime() const {
    uint32_t last_update_time = 0;
    for (const TimeWheel& wheel : wheels) {
        last_update_time |= (wheel.cur_slot_idx << wheel.bit_offset);
    }
    return last_update_time;
}

void TimeWheelTimerManager::tick() {
    TimeWheel& wheel = wheels[0];
    list<TimerPtr>& slot = wheel.slots[wheel.cur_slot_idx];
    while (!slot.empty()) {
        TimerPtr timer_ptr = move(slot.front());
        slot.pop_front();
        timer_ptr->slot_ptr = nullptr;
        timer_ptr->slot_it = decltype(timer_ptr->slot_it)();

        timer_ptr->callback();
    }
}

void TimeWheelTimerManager::moveToNextSlot(int level) {
    TimeWheel& wheel = wheels[level];
    if (++wheel.cur_slot_idx >= wheel.slots.size()) {
        wheel.cur_slot_idx = 0;

        // 发生进位了，要从更高级的轮加载到当前轮
        if (level + 1 < wheels.size()) {
            moveToNextSlot(level + 1);
        }
    }
    if (level <= 0) {
        return;
    }

    // 把本级当前的槽加载到低一级的轮
    list<TimerPtr>& slot = wheel.slots[wheel.cur_slot_idx];
    TimeWheel& lower_wheel = wheels[level - 1];
    while (!slot.empty()) {
        auto it = slot.begin();
        Timer& timer = **it;
        uint32_t lower_idx = lower_wheel.getIdx(timer.expire);
        list<TimerPtr>& lower_slot = lower_wheel.slots[lower_idx];

        lower_slot.splice(lower_slot.end(), slot, it);
        timer.slot_ptr = &lower_slot;
        // splice不会使迭代器失效，就不修改slot_it了
    }
}

void testTimeWheelTimerManager() {
    auto make_print_callback = [](int i) {
        return [=] {
            cout << i << '\n';
        };
    };

    TimeWheelTimerManager timer_mgr;
    auto update_to = [&](uint32_t cur_time) {
        cout << "cur_time = " << cur_time << '\n';
        timer_mgr.update(cur_time);
    };

    timer_mgr.addTimer(1, make_print_callback(1));
    timer_mgr.addTimer(1, make_print_callback(2));
    timer_mgr.addTimer(5, make_print_callback(3));
    update_to(1);

    // 空跑
    update_to(3);

    // 一次tick多个时间
    update_to(5);

    // 添加过去的时间，在下一次tick时调用
    timer_mgr.addTimer(1, make_print_callback(4));
    // update一个过去时间，等于update上次的时间
    update_to(0);

    // 添加到更高级的时间轮
    constexpr uint32_t LEVEL_1_TIME = 64;
    constexpr uint32_t LEVEL_2_TIME = 64 * 64;
    timer_mgr.addTimer(LEVEL_1_TIME - 1, make_print_callback(5));
    timer_mgr.addTimer(LEVEL_1_TIME, make_print_callback(6));
    timer_mgr.addTimer(LEVEL_1_TIME, make_print_callback(7));
    timer_mgr.addTimer(LEVEL_1_TIME + 1, make_print_callback(8));
    timer_mgr.addTimer(LEVEL_1_TIME + 1, make_print_callback(9));
    timer_mgr.addTimer(LEVEL_2_TIME, make_print_callback(10));
    auto cancellable_ptr = timer_mgr.addTimer(LEVEL_2_TIME + 1, make_print_callback(11));
    timer_mgr.addTimer(LEVEL_2_TIME + 1, make_print_callback(12));

    update_to(LEVEL_1_TIME);
    update_to(LEVEL_1_TIME + 1);

    // 先加载2级第一个槽
    update_to(LEVEL_2_TIME);
    // 测试移动到低级槽后再取消
    cancellable_ptr->cancel();
    cancellable_ptr->cancel();

    // 再添加一个过去的时间
    timer_mgr.addTimer(1, make_print_callback(13));
    update_to(LEVEL_2_TIME + 1);
}
