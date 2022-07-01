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
    CacheNode(const void *value, const std::string key, unsigned long long length): _value(malloc(length)), _key(key), _length(length) {
        memcpy(_value, value, length);
        _timestamp = (unsigned long long)time(NULL);
        _last = nullptr;
        _next = nullptr;
    }
    
    ~CacheNode() {
        free(_value);
    }
    
    void updateNode(const void *value, unsigned long long length){
        free(_value);
        _value = malloc(length);
        memcpy(_value, value, length);
        _length = length;
        _timestamp = (unsigned long long)time(NULL);
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
    void insertNodeAtHead(std::shared_ptr<CacheNode>node);
    void bringNodeToHead(std::shared_ptr<CacheNode>node);
    void removeNode(std::shared_ptr<CacheNode>node);
    std::shared_ptr<CacheNode> removeTailNode();
    void removeAll();
    
private:
    std::shared_ptr<CacheNode> _head;
    std::shared_ptr<CacheNode> _tail;
    
};

class WMemoryCache {
    
public:
    WMemoryCache(size_t count, size_t cost):
    _lru(std::make_shared<LinkedMap>()), countLimit(count), costLimit(cost), countCur(0), costCur(0) {};
    
    std::pair<const void *, int> get(const std::string key);
    void set(const void *value, const std::string key, size_t cost);
    bool contain(const std::string key);
    void removeObj(const std::string key);
    void removeAllObj();
    
private:
    std::shared_ptr<LinkedMap> _lru;
    const size_t countLimit;
    const size_t costLimit;
    size_t countCur;
    size_t costCur;
    std::mutex _mutex;
    
    std::shared_ptr<CacheNode>_get(const std::string key);
    void _trimToFitCost();
    void _trimToFitCount();
};

};



#endif /* WMemoryCache_hpp */
