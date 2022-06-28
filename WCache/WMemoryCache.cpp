//
//  WMemoryCache.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include "WMemoryCache.hpp"

namespace WMCache {

void LinkedMap::insertNodeAtHead(std::shared_ptr<CacheNode> node) {
    if (node == nullptr) {
        return;
    }
    auto p = _map.insert({node->_key, node});
    if (!p.second) {
        p.first->second = node;
        // TODO 更新coast
        bringNodeToHead(node);
        return;
    }
    costLimit += node->_cost;
    countLimit++;
    if (_head) {
        node->_next = _head;
        _head->_last = node;
        _head = node;
    } else {
        _head = _tail = node;
    }
}

void LinkedMap::bringNodeToHead(std::shared_ptr<CacheNode> node) {
    if (node == nullptr || _head == node) {
        return;
    }
    if (_tail == node) {
        _tail = node->_last;
        _tail->_next = nullptr;
    } else {
        node->_next->_last = node->_last;
        node->_last->_next = node->_next;
    }
    node->_next = _head;
    node->_last = nullptr;
    _head->_last = node;
    _head = node;
}

void LinkedMap::removeNode(std::shared_ptr<CacheNode> node) {
    if (node == nullptr) {
        return;
    }
    _map.erase(node->_key);
    costLimit -= node->_cost;
    countLimit--;
    if (node->_next) {
        node->_next->_last = node->_last;
    }
    if (node->_last) {
        node->_last->_next = node->_next;
    }
    if (_head == node) {
        _head = node->_next;
    }
    if (_tail == node) {
        _tail = node->_last;
    }
}

void LinkedMap::removeAll() {
    costLimit = 0;
    countLimit = 0;
    _head = nullptr;
    _tail = nullptr;
    _map.clear();
}

std::shared_ptr<CacheNode> WMemoryCache::_get(const std::string key) {
    _mutex.lock();
    auto it = _lru->_map.find(key);
    _mutex.unlock();
    if (it == _lru->_map.cend()) {
        return nullptr;
    }
    it->second->_timestamp = (unsigned long long)time(NULL);
    return it->second;
}

void *WMemoryCache::get(const std::string key) {
    auto p = _get(key);
    if (p == nullptr) {
        return NULL;
    }
    _lru->bringNodeToHead(p);
    return p->_value;
}

void WMemoryCache::set(void *value, const std::string key) {
    set(value, key, 0);
}

void WMemoryCache::set(void *value, const std::string key, size_t cost) {
    if (value == nullptr) {
        removeObj(key);
        return;
    }
    _mutex.lock();
    auto node = std::make_shared<CacheNode>(value, key, cost);
    auto it = _lru->_map.find(key);
    if (it == _lru->_map.cend()) {
        _lru->insertNodeAtHead(node);
    } else {
        _lru->bringNodeToHead(it->second);
    }
    _mutex.unlock();
}

bool WMemoryCache::contain(const std::string key) {
    _mutex.lock();
    auto it = _lru->_map.find(key);
    _mutex.unlock();
    if (it == _lru->_map.cend()) {
        return false;
    }
    return true;
}

void WMemoryCache::removeObj(const std::string key) {
    auto p = _get(key);
    if (p == nullptr) {
        return;
    }
    _mutex.lock();
    _lru->removeNode(p);
    _mutex.unlock();
}

void WMemoryCache::removeAllObj() {
    _mutex.lock();
    _lru->removeAll();
    _mutex.unlock();
}

};
