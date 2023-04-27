#pragma once

#include "common_include.h"

template<class Key, class Value>
class LruCache {
private:
    struct Item {
        Key key;
        Value value;
    };

    // 存储数据的链表，头部是最近使用的，尾部是最久未使用的
    list<Item> items;
    using ItemsIt = typename decltype(items)::iterator;
    // key -> items索引
    unordered_map<Key, ItemsIt> key_items_it_map;

    const int max_size = 0;

public:
    LruCache(int _max_size) : max_size(_max_size) {
        assert(max_size > 0);
    }

    void set(const Key& key, const Value& value) {
        auto it = key_items_it_map.find(key);
        if (it != key_items_it_map.end()) {
            // key存在，修改value
            it->second->value = value;
            moveItemToFront(it->second);
            return;
        }

        // key不存在，先删除最久未使用的，再添加
        while (key_items_it_map.size() >= max_size) {
            const Key& key_to_del = items.back().key;
            key_items_it_map.erase(key_to_del);
            items.pop_back();
        }

        items.push_front({key, value});
        key_items_it_map[key] = items.begin();
    }

    Value* get(const Key& key) {
        auto it = key_items_it_map.find(key);
        if (it == key_items_it_map.end()) {
            return nullptr;
        }
        moveItemToFront(it->second);
        return &it->second->value;
    }

private:
    void moveItemToFront(ItemsIt items_it) {
        // 推荐的方法，直接操作链表指针，不会使迭代器失效
        items.splice(items.begin(), items, items_it);

        // 另一种方法，由于用到erase，会使迭代器失效，记得修改map
        // items.emplace_front(move(*items_it));
        // items.erase(items_it);
        // auto new_items_it = items.begin();
        // key_items_it_map[new_items_it->key] = new_items_it;
    }
};

void testLruCache();

template<class Key, class Value>
class LfuCache {
private:
    using ValuePtr = unique_ptr<Value>;

    struct FreqGroup {
        int freq = 1;
        // key -> value指针
        unordered_map<Key, ValuePtr> key_value_ptr_map;
    };

    // 频率组链表，头部是频率最小的，尾部是频率最大的
    list<FreqGroup> groups;
    using GroupsIt = typename decltype(groups)::iterator;
    // key -> 频率组索引
    unordered_map<Key, GroupsIt> key_groups_it_map;

    const int max_size = 0;

public:
    LfuCache(int _max_size) : max_size(_max_size) {
        assert(max_size > 0);
    }

    void set(const Key& key, const Value& value) {
        auto it = key_groups_it_map.find(key);
        if (it != key_groups_it_map.end()) {
            // key存在，修改value
            *it->second->key_value_ptr_map[key] = value;
            incFreq(it->second, key);
            return;
        }

        // key不存在，先删除频率最小的，再添加
        while (key_groups_it_map.size() >= max_size) {
            FreqGroup& group_to_del = groups.front();

            auto value_it_to_del = group_to_del.key_value_ptr_map.begin();
            key_groups_it_map.erase(value_it_to_del->first);
            group_to_del.key_value_ptr_map.erase(value_it_to_del);

            if (group_to_del.key_value_ptr_map.empty()) {
                groups.pop_front();
            }
        }

        // 组频率 = 1，没有则添加
        if (groups.empty() || groups.front().freq != 1) {
            groups.emplace_front();
        }
        FreqGroup& group = groups.front();
        group.key_value_ptr_map.emplace(key, make_unique<Value>(value));

        key_groups_it_map[key] = groups.begin();
    }

    Value* get(const Key& key) {
        auto it = key_groups_it_map.find(key);
        if (it == key_groups_it_map.end()) {
            return nullptr;
        }
        Value* res = it->second->key_value_ptr_map[key].get();
        incFreq(it->second, key);
        return res;
    }

    int getFreq(const Key& key) const {
        auto it = key_groups_it_map.find(key);
        if (it == key_groups_it_map.end()) {
            return 0;
        }
        return it->second->freq;
    }

private:
    void incFreq(GroupsIt groups_it, const Key& key) {
        FreqGroup& group = *groups_it;
        GroupsIt next_groups_it = groups_it;
        ++next_groups_it;
        // 新组频率 = 旧组频率 + 1，没有则添加
        if (next_groups_it == groups.end() || next_groups_it->freq != group.freq + 1) {
            // 优化，如果旧组内只有一个key，直接增加频率
            if (group.key_value_ptr_map.size() == 1) {
                group.freq++;
                return;
            }

            next_groups_it = groups.emplace(next_groups_it);
            next_groups_it->freq = group.freq + 1;
        }
        FreqGroup& next_group = *next_groups_it;

        // 移动到新组
        auto old_value_it = group.key_value_ptr_map.find(key);
        next_group.key_value_ptr_map.emplace(key, move(old_value_it->second));
        group.key_value_ptr_map.erase(old_value_it);
        if (group.key_value_ptr_map.empty()) {
            groups.erase(groups_it);
        }

        key_groups_it_map[key] = next_groups_it;
    }
};

void testLfuCache();
