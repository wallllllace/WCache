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
    CacheNode(void *value, const std::string key, unsigned long long length): _value(value), _key(key), _length(length) {
        _value = malloc(length);
        memcpy(_value, value, length);
        _timestamp = (unsigned long long)time(NULL);
        _last = nullptr;
        _next = nullptr;
    }
    ~CacheNode() {
        free(_value);
    }
    void * _value;
    size_t _length;
    const std::string _key;
    unsigned long long _timestamp;
    std::shared_ptr<CacheNode> _last;
    std::shared_ptr<CacheNode> _next;
};


class LinkedMap {
public:
    std::unordered_map<std::string, std::shared_ptr<CacheNode>> _map;
    
    LinkedMap(size_t count, size_t cost): countLimit(count), costLimit(cost) {}
    void insertNodeAtHead(std::shared_ptr<CacheNode>node);
    void bringNodeToHead(std::shared_ptr<CacheNode>node);
    void removeNode(std::shared_ptr<CacheNode>node);
    void removeAll();
    
private:
    std::shared_ptr<CacheNode> _head;
    std::shared_ptr<CacheNode> _tail;
    size_t countLimit;
    size_t costLimit;
};

class WMemoryCache {
    
public:
    WMemoryCache(size_t count = 100, size_t cost = 10 * 1024 * 1024): _lru(std::make_shared<LinkedMap>(LinkedMap(count, cost))) {};
    
    void *get(const std::string key);
    void set(void *value, const std::string key, size_t cost);
    bool contain(const std::string key);
    void removeObj(const std::string key);
    void removeAllObj();
    
private:
    std::shared_ptr<LinkedMap> _lru;
    std::mutex _mutex;
    std::shared_ptr<CacheNode>_get(const std::string key);
};

};



#endif /* WMemoryCache_hpp */
