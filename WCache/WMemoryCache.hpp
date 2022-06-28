//
//  WMemoryCache.hpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#ifndef WMemoryCache_hpp
#define WMemoryCache_hpp

#include <stdio.h>
#include <unordered_map>
#include <list>
#include <memory>
#include <string>
#include <mutex>

namespace WMCache {

class CacheNode {
public:
    CacheNode(void *value, const std::string key, unsigned long long cost = 0): _value(value), _key(key), _cost(cost) {
        _timestamp = (unsigned long long)time(NULL);
        _last = nullptr;
        _next = nullptr;
    }
    void * _value;
    const std::string _key;
    unsigned long long _cost;
    unsigned long long _timestamp;
    CacheNode *_last;
    CacheNode *_next;
};


class LinkedMap {
public:
    std::unordered_map<std::string, CacheNode *> _map;
    
    LinkedMap(size_t count, size_t cost): countLimit(count), costLimit(cost) {}
    void insertNodeAtHead(CacheNode *node);
    void bringNodeToHead(CacheNode *node);
    void removeNode(CacheNode *node);
    void removeAll();
    
private:
    CacheNode * _head;
    CacheNode * _tail;
    size_t countLimit;
    size_t costLimit;
};

class WMemoryCache {
    
public:
    WMemoryCache(size_t count = 100, size_t cost = 10 * 1024 * 1024): _lru(std::make_shared<LinkedMap>(LinkedMap(count, cost))) {};
    
    void *get(const std::string key);
    void set(void *value, const std::string key);
    void set(void *value, const std::string key, size_t cost);
    bool contain(const std::string key);
    void removeObj(const std::string key);
    void removeAllObj();
    
private:
    std::shared_ptr<LinkedMap> _lru;
    std::mutex _mutex;
    CacheNode *_get(const std::string key);
};

};



#endif /* WMemoryCache_hpp */
