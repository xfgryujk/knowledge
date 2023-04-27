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
