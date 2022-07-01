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
        bringNodeToHead(node);
        return;
    }
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

std::shared_ptr<CacheNode> LinkedMap::removeTailNode() {
    if (_tail == nullptr) {
        return nullptr;
    }
    auto p = _tail;
    removeNode(p);
    return p;
}

void LinkedMap::removeAll() {
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

std::pair<const void *, int>WMemoryCache::get(const std::string key) {
    auto p = _get(key);
    if (p == nullptr) {
        return {NULL, 0};
    }
    _lru->bringNodeToHead(p);
    return {p->_value, p->_length};
}

void WMemoryCache::set(const void *value, const std::string key, size_t cost) {
    if (value == NULL) {
        removeObj(key);
        return;
    }
    _mutex.lock();
    auto it = _lru->_map.find(key);
    if (it == _lru->_map.cend()) {
        auto node = std::make_shared<CacheNode>(value, key, cost);
        _lru->insertNodeAtHead(node);
        ++countCur;
        costCur += cost;
    } else {
        size_t oldLen = it->second->_length;
        it->second->updateNode(value, cost);
        _lru->bringNodeToHead(it->second);
        costCur += cost - oldLen;
        if (costCur < 0) {
            costCur = 0;
        }
    }
    _mutex.unlock();
    if (countCur > countLimit) {
        _trimToFitCount();
    }
    if (costCur > costLimit) {
        _trimToFitCost();
    }
    
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
    --countCur;
    costCur -= p->_length;
    _mutex.unlock();
}

void WMemoryCache::removeAllObj() {
    _mutex.lock();
    _lru->removeAll();
    countCur = 0;
    costCur = 0;
    _mutex.unlock();
}


void WMemoryCache::_trimToFitCost() {
    if (costCur <= costLimit) {
        return;
    }
    while (costCur > costLimit) {
        _mutex.lock();
        auto node = _lru->removeTailNode();
        --countCur;
        costCur -= node->_length;
        _mutex.unlock();
    }
}

void WMemoryCache::_trimToFitCount() {
    if (countCur <= countLimit) {
        return;
    }
    while (countCur > countLimit) {
        _mutex.lock();
        auto node = _lru->removeTailNode();
        --countCur;
        costCur -= node->_length;
        _mutex.unlock();
    }
}


};
