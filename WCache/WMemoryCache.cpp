//
//  WMemoryCache.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include "WMemoryCache.hpp"

template <typename T>
WMemoryCache<T>::WMemoryCache(size_t count, size_t cost): countLimit(count), costLimit(cost) {
    currentCount = 0;
    currentCost = 0;
}

template <typename T>
std::shared_ptr<T> WMemoryCache<T>::get(const std::string key) {
    _mutex.lock();
    auto it = _map.find(key);
    _mutex.unlock();
    if (it == _map.cend()) {
        return nullptr;
    }
    return it->second;
}

template <typename T>
void WMemoryCache<T>::set(std::shared_ptr<T> sp, const std::string key) {
    _mutex.lock();
    auto p = _map.insert({key, sp});
    if (!p.second) {
        p.first->second = sp;
    }
    _mutex.unlock();
}

template <typename T>
void WMemoryCache<T>::set(std::shared_ptr<T> sp, const std::string key, size_t cost) {
    _mutex.lock();
    auto p = _map.insert({key, sp});
    if (!p.second) {
        p.first->second = sp;
    } else {
        currentCost += cost;
    }
    _mutex.unlock();
}

template <typename T>
bool WMemoryCache<T>::contain(const std::string key) {
    _mutex.lock();
    auto it = _map.find(key);
    _mutex.unlock();
    if (it == _map.cend()) {
        return false;
    }
    return true;
}

template <typename T>
void WMemoryCache<T>::removeObj(const std::string key) {
    _mutex.lock();
    _map.erase(key);
    _mutex.unlock();
}

template <typename T>
void WMemoryCache<T>::removeAllObj() {
    _mutex.lock();
    _map.clear();
    _mutex.unlock();
}
