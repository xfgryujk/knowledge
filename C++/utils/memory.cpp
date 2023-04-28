#include "memory.h"

constexpr size_t PTR_SIZE = sizeof(uintptr_t);

void* myMemcpyReverse(void* dst, const void* src, size_t num);

void* myMemmove(void* dst, const void* src, size_t num) {
    assert(dst != nullptr);
    assert(src != nullptr);
    if (num == 0 || dst == src) {
        return dst;
    }

    uint8_t* dst_byte_ptr = reinterpret_cast<uint8_t*>(dst);
    const uint8_t* src_byte_ptr = reinterpret_cast<const uint8_t*>(src);

    // 有重叠且dst在src后面，则从后往前拷贝
    if (src_byte_ptr < dst_byte_ptr && dst_byte_ptr < src_byte_ptr + num) {
        myMemcpyReverse(dst, src, num);
    } else {
        myMemcpy(dst, src, num);
    }
    return dst;
}

void* myMemcpy(void* dst, const void* src, size_t num) {
    assert(dst != nullptr);
    assert(src != nullptr);
    if (num == 0 || dst == src) {
        return dst;
    }

    uint8_t* dst_byte_ptr = reinterpret_cast<uint8_t*>(dst);
    const uint8_t* src_byte_ptr = reinterpret_cast<const uint8_t*>(src);

    // 一次拷贝一字节直到dst对齐
    size_t tmp_num = reinterpret_cast<uintptr_t>(dst) % PTR_SIZE;
    if (tmp_num != 0) {
        tmp_num = PTR_SIZE - tmp_num;
        while (tmp_num > 0 && num > 0) {
            *(dst_byte_ptr++) = *(src_byte_ptr++);
            tmp_num--;
            num--;
        }
    }

    // 一次拷贝多字节
    uintptr_t* dst_int_ptr = reinterpret_cast<uintptr_t*>(dst_byte_ptr);
    const uintptr_t* src_int_ptr = reinterpret_cast<const uintptr_t*>(src_byte_ptr);
    while (num > PTR_SIZE) {
        *(dst_int_ptr++) = *(src_int_ptr++);
        num -= PTR_SIZE;
    }

    // 一次拷贝一字节
    dst_byte_ptr = reinterpret_cast<uint8_t*>(dst_int_ptr);
    src_byte_ptr = reinterpret_cast<const uint8_t*>(src_int_ptr);
    while (num > 0) {
        *(dst_byte_ptr++) = *(src_byte_ptr++);
        num--;
    }

    return dst;
}

void* myMemcpyReverse(void* dst, const void* src, size_t num) {
    // 为了方便处理，这里指针指向上一次拷贝的位置
    uint8_t* dst_byte_ptr = reinterpret_cast<uint8_t*>(dst) + num;
    const uint8_t* src_byte_ptr = reinterpret_cast<const uint8_t*>(src) + num;

    // 一次拷贝一字节直到dst对齐
    size_t tmp_num = reinterpret_cast<uintptr_t>(dst_byte_ptr) % PTR_SIZE;
    while (tmp_num > 0 && num > 0) {
        *(--dst_byte_ptr) = *(--src_byte_ptr);
        tmp_num--;
        num--;
    }

    // 一次拷贝多字节
    uintptr_t* dst_int_ptr = reinterpret_cast<uintptr_t*>(dst_byte_ptr);
    const uintptr_t* src_int_ptr = reinterpret_cast<const uintptr_t*>(src_byte_ptr);
    while (num > PTR_SIZE) {
        *(--dst_int_ptr) = *(--src_int_ptr);
        num -= PTR_SIZE;
    }

    // 一次拷贝一字节
    dst_byte_ptr = reinterpret_cast<uint8_t*>(dst_int_ptr);
    src_byte_ptr = reinterpret_cast<const uint8_t*>(src_int_ptr);
    while (num > 0) {
        *(--dst_byte_ptr) = *(--src_byte_ptr);
        num--;
    }

    return dst;
}

void testMemmove() {
    string origin = "012345678901234567890123456789";

    // 不重叠
    string buffer = origin;
    myMemmove(&buffer[15], &buffer[0], 10);
    assert(buffer == "012345678901234012345678956789");

    // 重叠，dst在src前面
    buffer = origin;
    myMemmove(&buffer[0], &buffer[1], 25);
    assert(buffer == "123456789012345678901234556789");

    // 重叠，dst在src后面
    buffer = origin;
    myMemmove(&buffer[1], &buffer[0], 25);
    assert(buffer == "001234567890123456789012346789");
}

char* myStrcpy(char* dst, const char* src) {
    assert(dst != nullptr);
    assert(src != nullptr);
    if (dst == src) {
        return dst;
    }

    char* old_dst = dst;
    while (*src != '\0') {
        *(dst++) = *(src++);
    }
    *dst = '\0';
    return old_dst;
}

void testStrcpy() {
    string buffer(10, '\0');
    myStrcpy(&buffer[0], "0123456789");
    assert(buffer == "0123456789");
}

FixedSizeMemoryPool::FixedSizeMemoryPool(size_t _block_size) :
    block_size(max(_block_size, sizeof(Block))),
    chunk_size(max(block_size, 2 * 1024ul)) {
}

void* FixedSizeMemoryPool::allocate() {
    if (free_head_ptr == nullptr) {
        chunks.emplace_back(make_unique<uint8_t[]>(chunk_size));
        uint8_t* chunk_ptr = chunks.back().get();

        Block* prev_block_ptr = nullptr;
        for (int i = (chunk_size / block_size) - 1; i >= 0; i--) {
            Block* block_ptr = reinterpret_cast<Block*>(chunk_ptr + i * block_size);
            block_ptr->next_free_ptr = prev_block_ptr;
            prev_block_ptr = block_ptr;
        }
        free_head_ptr = prev_block_ptr;
    }

    Block* block_ptr = free_head_ptr;
    free_head_ptr = free_head_ptr->next_free_ptr;
    return block_ptr;
}

void FixedSizeMemoryPool::deallocate(void* ptr) {
    Block* block_ptr = reinterpret_cast<Block*>(ptr);
    block_ptr->next_free_ptr = free_head_ptr;
    free_head_ptr = block_ptr;
}

void testFixedSizeMemoryPool() {
    FixedSizeMemoryPool pool(100);

    char* ptr1 = static_cast<char*>(pool.allocate());
    char* ptr2 = static_cast<char*>(pool.allocate());
    strcpy(ptr1, "11111111111111111111");
    strcpy(ptr2, "22222222222222222222");
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);

    char* new_ptr2 = static_cast<char*>(pool.allocate());
    char* new_ptr1 = static_cast<char*>(pool.allocate());
    assert(new_ptr1 == ptr1);
    assert(new_ptr2 == ptr2);
    assert(new_ptr1[10] == '1');
    assert(new_ptr2[10] == '2');
    pool.deallocate(ptr1);
    pool.deallocate(ptr2);
}

void testSharedPtr() {
    SharedPtr<string> p1(new string("test"));
    assert(p1.use_count() == 1);
    assert(*p1 == "test");
    assert(p1->size() == 4);
    p1.reset();
    assert(!p1);

    SharedPtr<string> p2(new string("test"));
    p1 = p2;
    assert(p1.get() == p2.get());
    assert(p1.use_count() == 2);

    {
        SharedPtr<string> p3(move(p2));
        assert(!p2);
        assert(p3.get() == p1.get());
        assert(p3.use_count() == 2);
    }
    assert(p1.use_count() == 1);
}
