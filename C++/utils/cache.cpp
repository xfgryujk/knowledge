#include "cache.h"

void testLruCache() {
    LruCache<int, string> cache(3);
    cache.set(1, "1");
    cache.set(2, "2");
    cache.set(3, "3");

    // 取存在的key
    string* value_ptr = cache.get(1);
    assert(value_ptr != nullptr);
    assert(*value_ptr == "1");

    // 取不存在的key
    assert(cache.get(4) == nullptr);

    // 添加新key，2应该被淘汰
    cache.set(4, "4");
    assert(cache.get(2) == nullptr);
    value_ptr = cache.get(4);
    assert(value_ptr != nullptr);
    assert(*value_ptr == "4");

    // 修改存在的key
    cache.set(1, "10");
    value_ptr = cache.get(1);
    assert(value_ptr != nullptr);
    assert(*value_ptr == "10");
}

void testLfuCache() {
    LfuCache<int, string> cache(3);
    cache.set(1, "1");
    cache.set(2, "2");
    cache.set(3, "3");

    // 取存在的key
    string* value_ptr = cache.get(1);
    assert(value_ptr != nullptr);
    assert(*value_ptr == "1");

    // 取不存在的key
    assert(cache.get(4) == nullptr);

    // 取频率
    cache.get(1);
    cache.get(1);
    cache.get(3);
    assert(cache.getFreq(1) == 4);
    assert(cache.getFreq(2) == 1);
    assert(cache.getFreq(3) == 2);
    assert(cache.getFreq(4) == 0);

    // 添加新key，2应该被淘汰
    cache.set(4, "4");
    assert(cache.get(2) == nullptr);
    value_ptr = cache.get(4);
    assert(value_ptr != nullptr);
    assert(*value_ptr == "4");

    // 修改存在的key
    cache.set(1, "10");
    value_ptr = cache.get(1);
    assert(value_ptr != nullptr);
    assert(*value_ptr == "10");
    assert(cache.getFreq(1) == 6);
}
