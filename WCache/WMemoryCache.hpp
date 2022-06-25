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
#include <memory>
#include <string>
#include <mutex>

template <typename T>
class WMemoryCache {
    
public:
    WMemoryCache(size_t count = 100, size_t cost = 10 * 1024 * 1024);
    
    std::shared_ptr<T> get(const std::string key);
    void set(std::shared_ptr<T> sp, const std::string key);
    void set(std::shared_ptr<T> sp, const std::string key, size_t cost);
    bool contain(const std::string key);
    void removeObj(const std::string key);
    void removeAllObj();
    
private:
    std::unordered_map<std::string, std::shared_ptr<T>> _map;
    std::mutex _mutex;
    size_t countLimit;
    size_t costLimit;
    size_t currentCount;
    size_t currentCost;
};


#endif /* WMemoryCache_hpp */
