//
//  WCache.hpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#ifndef WCache_hpp
#define WCache_hpp

#include <stdio.h>
#include <string>
#include <sqlite3.h>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace WCache {
class WMemoryCache;
class WDiskCache;
class WCache {
    
public:
    WCache(const std::string& rootPath, const std::string& cacheName, size_t m_count_limit = 100, size_t m_cost_limit = 10*1024*1024, size_t d_count_limit = 1000, size_t d_cost_limit = 100*1024*1024);
    
    std::pair<const void *, int> get(const std::string& key);
    void set(const void *value, const std::string& key, size_t cost);
    bool contain(const std::string& key);
    void removeObj(const std::string& key);
    void removeAllObj();
    
private:
    std::shared_ptr<WMemoryCache> _memoryCache;
    std::shared_ptr<WDiskCache> _diskCache;
};


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
    
    std::pair<const void *, int> get(const std::string& key);
    void set(const void *value, const std::string& key, size_t cost);
    bool contain(const std::string& key);
    void removeObj(const std::string& key);
    void removeAllObj();
    
private:
    std::shared_ptr<LinkedMap> _lru;
    const size_t countLimit;
    const size_t costLimit;
    size_t countCur;
    size_t costCur;
    std::mutex _mutex;
    
    std::shared_ptr<CacheNode>_get(const std::string& key);
    void _trimToFitCost();
    void _trimToFitCount();
};


#define ROOTPATHLEN 500
#define TABLENAMELEN 100


class WDiskCache {
    
public:
    WDiskCache(const char *rootPath, const char *cacheName, size_t count, size_t cost);
    ~WDiskCache(){
        _db_close();
    }
    
    void set(const void *value, const std::string& key, size_t cost);
    std::pair<const void *, int> get(const std::string& key);
    bool contain(const std::string& key);
    void update(const std::string& key);
    void removeObj(const std::string& key);
    void removeObjs(std::vector<std::string>& keys);
    void removeAllObj();
    void trimToFitLimit();

private:
    char _rootPath[ROOTPATHLEN];
    char _cacheName[TABLENAMELEN];
    sqlite3 *_db = NULL;
    const size_t _countLimit;
    const size_t _costLimit;
    std::mutex _mutex;
    
    bool _db_open();
    void _db_close();
    void _trim_fit_count(size_t count);
    void _trim_fit_cost(size_t cost);
};

};

#endif /* WCache_hpp */
