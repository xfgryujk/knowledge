#pragma once

#include "common_include.h"

void* myMemmove(void* dst, const void* src, size_t num);
void* myMemcpy(void* dst, const void* src, size_t num);
void testMemmove();

char* myStrcpy(char* dst, const char* src);
void testStrcpy();

class FixedSizeMemoryPool {
private:
    struct Block {
        // 指向下一个未分配的内存块，只在这个Block未分配时有效
        Block* next_free_ptr;
    };

    // 内存块大小
    const size_t block_size = 0;
    // 一次申请的内存大小
    const size_t chunk_size = 0;

    // 未分配的内存块链表
    Block* free_head_ptr = nullptr;
    // 所有申请的内存链表
    list<unique_ptr<uint8_t[]>> chunks;

public:
    FixedSizeMemoryPool(size_t _block_size);

    void* allocate();
    void deallocate(void* ptr);
};

void testFixedSizeMemoryPool();

template<class T>
class SharedPtr {
private:
    struct ControlBlock {
        // 引用计数，使用原子操作保证线程安全
        atomic<int> ref_count = 1;
    };

    T* obj_ptr = nullptr;
    ControlBlock* control_ptr = nullptr;

public:
    SharedPtr() = default;

    SharedPtr(T* _obj_ptr) {
        reset(_obj_ptr);
    }

    SharedPtr(const SharedPtr& other) {
        *this = other;
    }

    SharedPtr(SharedPtr&& other) {
        *this = move(other);
    }

    ~SharedPtr() {
        reset();
    }

    void reset() {
        if (control_ptr == nullptr) {
            return;
        }

        // 原子操作保证只有一个线程看到ref_count变成0
        // 不保证同时读写同一个SharedPtr对象的线程安全，用户要自己保证this没有其他线程正在读写
        if (--control_ptr->ref_count == 0) {
            delete obj_ptr;
            delete control_ptr;
        }
        obj_ptr = nullptr;
        control_ptr = nullptr;
    }

    void reset(T* _obj_ptr) {
        reset();
        obj_ptr = _obj_ptr;
        control_ptr = obj_ptr == nullptr ? nullptr: new ControlBlock();
    }

    SharedPtr& operator=(const SharedPtr& other) {
        if (&other == this) {
            return *this;
        }

        reset();
        if (other.control_ptr == nullptr) {
            return *this;
        }

        // 此时至少还有other没被销毁，引用计数不会变成0，这里使用other.control_ptr是安全的
        // 不保证同时读写同一个SharedPtr对象的线程安全，用户要自己保证this、other没有其他线程正在读写
        ++other.control_ptr->ref_count;
        obj_ptr = other.obj_ptr;
        control_ptr = other.control_ptr;
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (&other == this) {
            return *this;
        }

        reset();
        if (other.control_ptr == nullptr) {
            return *this;
        }

        // 此时至少还有other没被销毁，引用计数不会变成0，这里使用other.control_ptr是安全的
        // 不保证同时读写同一个SharedPtr对象的线程安全，用户要自己保证this、other没有其他线程正在读写
        obj_ptr = other.obj_ptr;
        control_ptr = other.control_ptr;
        other.obj_ptr = nullptr;
        other.control_ptr = nullptr;
        return *this;
    }

    T* get() const {
        return obj_ptr;
    }

    T& operator*() const {
        return *obj_ptr;
    }

    T* operator->() const {
        return obj_ptr;
    }

    operator bool() const {
        return obj_ptr != nullptr;
    }

    int use_count() const {
        // 不保证同时读写同一个SharedPtr对象的线程安全，用户要自己保证this没有其他线程正在写入
        return control_ptr == nullptr ? 0 : control_ptr->ref_count.load();
    }
};

void testSharedPtr();
