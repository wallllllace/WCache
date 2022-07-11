//
//  WCache.m
//  WCache
//
//  Created by wangxiaorui19 on 2022/7/6.
//

#import "WCache.h"
//#include "WCache.hpp"
#import <Objc/runtime.h>
//#include <cstring>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>

#include <cstdio>
#include <fstream>
#include <thread>

#include <stdio.h>
//#include <string>
#include <sqlite3.h>
//#include <iostream>
#include <unordered_map>
#include <memory>
#include <mutex>

#define ROOTPATHLEN 500
#define TABLENAMELEN 100

namespace WCacheSpace {

class CacheNode {
public:
    CacheNode(const void *value, const std::string key, unsigned long long length): _value(value), _key(key), _length(length) {
        _timestamp = (unsigned long long)time(NULL);
        _last = nullptr;
        _next = nullptr;
    }
    
    ~CacheNode() {
        CFBridgingRelease(_value);
    }
    
    void updateNode(const void *value, unsigned long long length){
        CFBridgingRelease(_value);
        _value = value;
        _length = length;
        _timestamp = (unsigned long long)time(NULL);
    }
    
    const void * _value;
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

class WMemoryCache {
    
public:
    WMemoryCache(size_t count, size_t cost):
    _lru(std::make_shared<LinkedMap>()), countLimit(count), costLimit(cost), countCur(0), costCur(0) {};
    
    std::pair<const void *, int> get(const char *key);
    void set(const void *value, const char *key, size_t cost);
    bool contain(const char *key);
    void removeObj(const char *key);
    void removeAllObj();
    
private:
    std::shared_ptr<LinkedMap> _lru;
    const size_t countLimit;
    const size_t costLimit;
    size_t countCur;
    size_t costCur;
    std::mutex _mutex;
    
    std::shared_ptr<CacheNode>_get(const char *key);
    void _trimToFitCost();
    void _trimToFitCount();
};

std::shared_ptr<CacheNode> WMemoryCache::_get(const char *key) {
    auto it = _lru->_map.find(key);
    if (it == _lru->_map.cend()) {
        return nullptr;
    }
    it->second->_timestamp = (unsigned long long)time(NULL);
    return it->second;
}

std::pair<const void *, int>WMemoryCache::get(const char *key) {
    _mutex.lock();
    auto p = _get(key);
    if (p == nullptr) {
        _mutex.unlock();
        return {NULL, -1};
    }
    _lru->bringNodeToHead(p);
    _mutex.unlock();
    return {p->_value, p->_length};
}

void WMemoryCache::set(const void *value, const char *key, size_t cost) {
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
    if (countCur > countLimit) {
        _trimToFitCount();
    }
    if (costCur > costLimit) {
        dispatch_async(dispatch_get_global_queue(0, 0), ^{
            _trimToFitCost();
        });
    }
    _mutex.unlock();
}

bool WMemoryCache::contain(const char *key) {
    _mutex.lock();
    auto it = _lru->_map.find(key);
    _mutex.unlock();
    if (it == _lru->_map.cend()) {
        return false;
    }
    return true;
}

void WMemoryCache::removeObj(const char *key) {
    _mutex.lock();
    auto p = _get(key);
    if (p == nullptr) {
        return;
    }
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
    bool finish = false;
    while (!finish) {
        if (_mutex.try_lock()) {
            if (costCur > costLimit) {
                auto node = _lru->removeTailNode();
                --countCur;
                costCur -= node->_length;
            } else {
                finish = YES;
            }
            _mutex.unlock();
        } else {
            usleep(10 * 1000); //10 ms
        }
    }
}

void WMemoryCache::_trimToFitCount() {
    if (countCur <= countLimit) {
        return;
    }
    auto node = _lru->removeTailNode();
    --countCur;
    costCur -= node->_length;
}

static const char * k_db_name = "w_cache.sqlite";

#define WCacheLock() dispatch_semaphore_wait(_lock, DISPATCH_TIME_FOREVER)
#define WCacheUnlock() dispatch_semaphore_signal(_lock)

#ifdef DEBUG
static bool k_log_enable = true;
#else
static bool K_log_enable = false;
#endif

class WDiskCache {
    
public:
    WDiskCache(const char *rootPath, const char *cacheName, size_t count, size_t cost);
    ~WDiskCache(){
        _db_close();
    }
    
    void set(const void *value, const char *key, size_t cost);
    std::pair<const void *, int> get(const char *key);
    bool contain(const char *key);
    void updateTime(const char *key);
    void removeObj(const char *key);
    void removeObjs(std::vector<std::string>& keys);
    void removeAllObj();
    void trimToFitLimit();

private:
    char _rootPath[ROOTPATHLEN];
    char _cacheName[TABLENAMELEN];
    char _db_path[ROOTPATHLEN+TABLENAMELEN+1];
    sqlite3 *_db = NULL;
    std::unordered_map<const char *, sqlite3_stmt *> _dbStmtCache;
    const size_t _countLimit;
    const size_t _costLimit;
//    std::mutex _mutex;
    dispatch_semaphore_t _lock = dispatch_semaphore_create(1);
    
    bool _db_open();
    bool _db_initialize();
    void _db_close();
    void _db_reset();
    bool _db_check();
    void _trim_fit_count(size_t count);
    void _trim_fit_cost(size_t cost);
    
    sqlite3_stmt * _dbPrepareStmt(const char *sql);
    bool _db_save(const char * key, const void * value, size_t size);
    bool _db_update(const char * key, const void * value, size_t size);
    bool _db_updateAccessTime(const char *key);
    bool _db_delete(const char * key);
    bool _db_deleteItems(std::vector<std::string> &keys);
    std::pair<const void *, int>_db_get(const char *key);
    void _db_getItem(const char *key);
    unsigned int _db_getCount(const char *key);
    unsigned int _db_getTotalCount();
    unsigned int _db_getTotalCost();
    unsigned int _db_getTotal(const char *key);
    std::vector<std::pair<const char *, int>> _db_getItemByTimeAsc(unsigned int count);
};

WDiskCache::WDiskCache(const char *rootPath, const char *cacheName, size_t count, size_t cost) : _countLimit(count), _costLimit(cost) {
    if (strlen(rootPath) > ROOTPATHLEN) {
        throw "rootPath too long";
    }
    strcpy(_rootPath, rootPath);
    if (strlen(cacheName) > TABLENAMELEN) {
        throw "tableName too long";
    }
    strcpy(_cacheName, cacheName);
    
    strcpy(_db_path, _rootPath);
    strcat(_db_path, "/");
    strcat(_db_path, k_db_name);
    if (k_log_enable) {
        std::cout << "rootpath长度：" << strlen(_db_path) <<std::endl;
    }
    if (!_db_open() || !_db_initialize()) {
        _db_close();
        _db_reset();
        if (!_db_open() || !_db_initialize()) {
            _db_close();
        }
    }
};

// 增、改
void WDiskCache::set(const void *value, const char * key, size_t cost) {
    WCacheLock();
    bool result = _db_save(key, value, cost);
    WCacheUnlock();
    if (k_log_enable) {
        if (result) {
            std::cout << "插入成功，key:" << key << std::endl;
        } else {
            std::cout << "插入失败，key:" << key << std::endl;
        }
    }
}

// 查
std::pair<const void *, int> WDiskCache::get(const char *key) {
    WCacheLock();
    auto p = _db_get(key);
    _db_updateAccessTime(key);
    WCacheUnlock();
    return p;
}

bool WDiskCache::contain(const char *key) {
    WCacheLock();
    unsigned int result = _db_getCount(key);
    WCacheUnlock();
    if (result == -1) {
        if (k_log_enable) {
            std::cout << "查询数量失败，key:" << key << std::endl;
        }
        return false;
    } else if(result > 0) {
        return true;
    }
    return false;
}

void WDiskCache::updateTime(const char *key) {
    WCacheLock();
    _db_updateAccessTime(key);
    WCacheUnlock();
}

void WDiskCache::removeObj(const char *key) {
    WCacheLock();
    bool result = _db_delete(key);
    WCacheUnlock();
    if (k_log_enable) {
        if (result) {
            std::cout << "删除成功，key:" << key << std::endl;
        } else {
            std::cout << "删除失败，key:" << key << std::endl;
        }
    }
}

void WDiskCache::removeObjs(std::vector<std::string> &keys) {
    WCacheLock();
    bool result = _db_deleteItems(keys);
    WCacheUnlock();
    if (k_log_enable) {
        if (result) {
            std::cout << "批量删除成功" << std::endl;
        } else {
            std::cout << "批量删除失败" << std::endl;
        }
    }
}

void WDiskCache::removeAllObj() {
    WCacheLock();
    _db_close();
    WCacheUnlock();
    int result = std::remove(_rootPath);
    if (k_log_enable){
        if (result == 0) {
            std::cout << "删除数据库成功" << std::endl;
        } else {
            std::cout << "删除数据库失败" << std::endl;
        }
    }
}

void WDiskCache::trimToFitLimit() {
    if (_countLimit > 0) {
        WCacheLock();
        _trim_fit_count(_countLimit);
        WCacheUnlock();
    }
    if (_costLimit > 0) {
        WCacheLock();
        _trim_fit_cost(_costLimit);
        WCacheUnlock();
    }
}

bool WDiskCache::_db_open() {
    if (_db) {
        return true;
    }
    int result = sqlite3_open(_db_path, &_db);
    if (result == SQLITE_OK) {
        return true;
    } else {
        _db = NULL;
        if (k_log_enable) {
            std::cout << "sqlite open failed" << std::endl;
        }
        return false;
    }
}


bool WDiskCache::_db_initialize() {
    const char *sql = "pragma journal_mode = wal; pragma synchronous = normal; create table if not exists w_cache (key text, value blob, size integer, modification_time integer, last_access_time integer, primary key(key)); create index if not exists last_access_time_idx on w_cache(last_access_time);";
    char * error = NULL;
    int result = sqlite3_exec(_db, sql, NULL, NULL, &error);
    if (error) {
        if (k_log_enable) {
            std::cout << "表创建失败" << std::endl;
        }
        sqlite3_free(error);
        return false;
    }
    return result == SQLITE_OK;
}

void WDiskCache::_db_close() {
    if (!_db) {
        return;
    }
    int  result = 0;
    bool retry = false;
    bool stmtFinalized = false;
   
    do {
        retry = false;
        result = sqlite3_close(_db);
        if (result == SQLITE_BUSY || result == SQLITE_LOCKED) {
            if (!stmtFinalized) {
                stmtFinalized = true;
                sqlite3_stmt *stmt;
                while ((stmt = sqlite3_next_stmt(_db, NULL)) != 0) {
                    sqlite3_finalize(stmt);
                    retry = true;
                }
            }
        } else if (result != SQLITE_OK) {
            if (k_log_enable) {
                std::cout << "sqlite close failed" << std::endl;
            }
        }
    } while (retry);
    _db = NULL;
}

void WDiskCache::_db_reset() {
    int result = std::remove(_db_path);
    if (k_log_enable) {
        if (result == 0) {
            std::cout << "删除数据库成功" << std::endl;
        } else {
            std::cout << "删除数据库失败" << std::endl;
        }
    }
}

bool WDiskCache::_db_check() {
    if (!_db) {
        return _db_open() && _db_initialize();
    }
    return true;
}

void WDiskCache::_trim_fit_count(size_t maxCount) {
    unsigned int total = _db_getTotalCount();
    if (maxCount > total) {
        return;
    }
    bool suc = false;
    do {
        unsigned int perCount = 16;
        auto v = _db_getItemByTimeAsc(perCount);
        for (auto &item : v) {
            if (total > maxCount) {
                suc = _db_delete(item.first);
                total--;
            } else {
                break;
            }
            if (!suc) {
                break;
            }
        }
    } while (total > maxCount && suc);
    if (suc && _db) {
        sqlite3_wal_checkpoint(_db, NULL);
    }
    return;
}

void WDiskCache::_trim_fit_cost(size_t maxCost) {
    unsigned int total = _db_getTotalCost();
    if (maxCost > total) {
        return;
    }
    bool suc = false;
    do {
        unsigned int perCount = 16;
        auto v = _db_getItemByTimeAsc(perCount);
        for (auto &item : v) {
            if (total > maxCost) {
                suc = _db_delete(item.first);
                total -= item.second;
            } else {
                break;
            }
            if (!suc) {
                break;
            }
        }
    } while (total > maxCost && suc);
    if (suc && _db) {
        sqlite3_wal_checkpoint(_db, NULL);
    }
    return;
}

sqlite3_stmt * WDiskCache::_dbPrepareStmt(const char *sql) {
    if (!sql || !_db_check()) {
        return NULL;
    }
    sqlite3_stmt *stmt = NULL;
    auto p = _dbStmtCache.find(sql);
    if (p == _dbStmtCache.cend()) {
        int result = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
        if (result != SQLITE_OK) {
            if (k_log_enable) {
                std::cout << "sqlite stmt prepare error" << std::endl;
            }
            return NULL;
        }
        _dbStmtCache.insert({sql, stmt});
    } else {
        stmt = p->second;
        sqlite3_reset(stmt);
    }
    return stmt;
}

bool WDiskCache::_db_save(const char * key, const void * value, size_t size) {
    const char * sql = "insert or replace into w_cache (value, key, size, modification_time, last_access_time) values (?1, ?2, ?3, ?4, ?5);";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return false;
    }
    int timestamp = (int)time(NULL);
    sqlite3_bind_blob(stmt, 1, value, (int)size, 0);
    sqlite3_bind_text(stmt, 2, key, -1, NULL);
    sqlite3_bind_int(stmt, 3, (int)size);
    sqlite3_bind_int(stmt, 4, timestamp);
    sqlite3_bind_int(stmt, 5, timestamp);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        if (k_log_enable) {
            std::cout << "sqlite stmt save error" << std::endl;
        }
        return false;
    }
    return true;
}

bool WDiskCache::_db_update(const char * key, const void * value, size_t size) {
    const char * sql = "update w_cache set value = ?1, size = ?2, modification_time = ?3, last_access_time = ?4 where key = ?5;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return false;
    }
    int timestamp = (int)time(NULL);
    sqlite3_bind_blob(stmt, 1, value, (int)size, 0);
    sqlite3_bind_int(stmt, 2, (int)size);
    sqlite3_bind_int(stmt, 3, timestamp);
    sqlite3_bind_int(stmt, 4, timestamp);
    sqlite3_bind_text(stmt, 5, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        if (k_log_enable) {
            std::cout << "sqlite update error" << std::endl;
        }
        return false;
    }
    return true;
}

bool WDiskCache::_db_updateAccessTime(const char *key) {
    const char *sql = "update w_cache set last_access_time = ?1 where key = ?2;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return false;
    }
    int timestamp = (int)time(NULL);
    sqlite3_bind_int(stmt, 1, timestamp);
    sqlite3_bind_text(stmt, 2, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        if (k_log_enable) {
            std::cout << "sqlite update last_access_time error" << std::endl;
        }
        return false;
    }
    return true;
}

bool WDiskCache::_db_delete(const char * key) {
    const char *sql = "delete from w_cache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        if (k_log_enable) {
            std::cout << " db delete error" << std::endl;
        }
        return false;
    }
    return true;
}

bool WDiskCache::_db_deleteItems(std::vector<std::string> &keys) {
    if (keys.size() == 0) {
        return false;
    }
    std::string s("");
    for (decltype(keys.size()) i = 0, max = keys.size(); i < max; ++i) {
        s += "?";
        s += std::to_string(i+1);
        if (i + 1 != max) {
            s += ",";
        }
    }
    std::string sql = "delete from w_cache where key in (";
    sql += s + ");";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql.c_str());
    if (!stmt) {
        return false;
    }
    for (decltype(keys.size()) i = 0, max = keys.size(); i < max; ++i){
        auto key = keys[i];
        sqlite3_bind_text(stmt, (int)i + 1, key.c_str(), -1, NULL);
    }
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (result == SQLITE_ERROR) {
        if (k_log_enable) {
            std::cout << "sqlite delete items error" << std::endl;
        }
        return false;
    }
    return true;
}


std::pair<const void *, int> WDiskCache::_db_get(const char *key) {
    const char *sql = "select value from w_cache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return {NULL, 0};
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        const void *value = sqlite3_column_blob(stmt, 0);
        int value_bytes = sqlite3_column_bytes(stmt, 0);
        return {value, value_bytes};
    } else {
        if (result != SQLITE_DONE) {
            if (k_log_enable) {
                std::cout << "sqlite query error" << std::endl;
            }
        }
        return {NULL, 0};
    }
}

void WDiskCache::_db_getItem(const char *key) {
    const char *sql = "select key, value, size, modification_time, last_access_time from w_cache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        
    } else {
        if (result != SQLITE_DONE) {
            if (k_log_enable) {
                std::cout << "sqlite query error" << std::endl;
            }
        }
        return;
    }
}

unsigned int WDiskCache::_db_getCount(const char *key) {
    const char *sql = "select count(key) from w_cache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        if (k_log_enable) {
            std::cout << "sqlite query error" << std::endl;
        }
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

unsigned int WDiskCache::_db_getTotalCount() {
    const char *sql = "select count(*) from w_cache;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return -1;
    }
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        if (k_log_enable) {
            std::cout << "sqlite query totalCount error" << std::endl;
        }
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

unsigned int WDiskCache::_db_getTotalCost() {
    const char *sql = "select sum(size) from w_cache;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return -1;
    }
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        if (k_log_enable) {
            std::cout << "sqlite query totalCost error" << std::endl;
        }
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

unsigned int WDiskCache::_db_getTotal(const char *key){
    const char *sql = "select sum(size) from w_cache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        if (k_log_enable) {
            std::cout << "sqlite query error" << std::endl;
        }
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

std::vector<std::pair<const char *, int>> WDiskCache::_db_getItemByTimeAsc(unsigned int count) {
    const char *sql = "select key, size from w_cache order by last_access_time asc limit ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(sql);
    if (!stmt) {
        return {};
    }
    std::vector<std::pair<const char *, int>> v{};
    do {
        int result = sqlite3_step(stmt);
        if (result == SQLITE_ROW) {
            const char *key = (const char *)sqlite3_column_text(stmt, 0);
            unsigned int size = sqlite3_column_int(stmt, 1);
            v.push_back({key, size});
        } else if (result == SQLITE_DONE) {
            break;
        } else {
            if (k_log_enable) {
                std::cout << "sqlite getItemByTimeAsc error" << std::endl;
            }
            break;
        }
    } while (1);
    return v;
}

};

@implementation WCache{
    WCacheSpace::WMemoryCache * _memorycache;
    WCacheSpace::WDiskCache * _diskcache;
}

- (instancetype)initWithPath:(NSString *)path cacheName:(NSString *)cacheName {
    return [self initWithPath:path cacheName:cacheName memoryCountLimit:1000 memoryCostLimit:10*1024*1024 diskCountLimit:10000 diskCostLimit:100*1024*1024];
}

- (instancetype)initWithPath:(NSString *)path
                   cacheName:(NSString *)cacheName
            memoryCountLimit:(unsigned long long)memoryCountLimit
             memoryCostLimit:(unsigned long long)memoryCostLimit
              diskCountLimit:(unsigned long long)diskCountLimit
               diskCostLimit:(unsigned long long)diskCostLimit  {
    self = [super init];
    if (self) {
        _memorycache = new WCacheSpace::WMemoryCache(memoryCountLimit, memoryCostLimit);
        try {
            _diskcache = new WCacheSpace::WDiskCache([path UTF8String], [cacheName UTF8String], diskCountLimit, diskCostLimit);
        } catch (const char *msg) {
            std::cout << "exception: " << msg << std::endl;
        }
    }
    return self;
}

//
- (BOOL)containsObjectForKey:(NSString *)key {
    if (!key || key.length == 0) {
        return NO;
    }
    const char *k = [key UTF8String];
    return _memorycache->contain(k) || _diskcache->contain(k);
}

- (nullable id)objectForKey:(NSString *)key {
    if (!key || key.length == 0) {
        return nil;
    }
    const char *k = [key UTF8String];
    auto p = _memorycache->get(k);
    if (p.second == -1 || p.first == NULL) {
        p = _diskcache->get(k);
        if (p.second != 0) {
            NSData *data = [NSData dataWithBytes:p.first length:p.second];
            id object = [NSKeyedUnarchiver unarchiveObjectWithData:data];
            _memorycache->set((__bridge_retained void *)object, k, 0);
            return object;
        }
    } else {
        _diskcache->updateTime(k);
    }
    return (__bridge id)p.first;
}

- (void)setObject:(nullable id)object forKey:(NSString *)key {
    if (!key || key.length == 0) {
        return ;
    }
    if (!object) {
        return;
    }
    const char *k = [key UTF8String];
    _memorycache->set((__bridge_retained void *)object, k, 0);
    NSData *value = [NSKeyedArchiver archivedDataWithRootObject:object];
    _diskcache->set(value.bytes, k, value.length);
}

- (void)setObject:(nullable id)object forKey:(NSString *)key withCost:(NSUInteger)cost {
    if (!key || key.length == 0) {
        return ;
    }
    if (!object) {
        return;
    }
    const char *k = [key UTF8String];
    _memorycache->set((__bridge_retained void *)object, k, cost);
    NSData *value = [NSKeyedArchiver archivedDataWithRootObject:object];
    _diskcache->set(value.bytes, k, value.length);
}

- (void)removeObjectForKey:(NSString *)key {
    if (!key || key.length == 0) {
        return ;
    }
    const char *k = [key UTF8String];
    _memorycache->removeObj(k);
    _diskcache->removeObj(k);
}

- (void)removeAllObjects {
    _memorycache->removeAllObj();
    _diskcache->removeAllObj();
}

- (void)trimBackground {
    _diskcache->trimToFitLimit();
}

- (void)dealloc {
    delete _memorycache;
    if (_diskcache) {
        delete _diskcache;
    }
}

#pragma mark - test code

- (void)m_setObject:(nullable id)object forKey:(NSString *)key {
    if (!key || key.length == 0) {
        return ;
    }
    _memorycache->set((__bridge_retained void *)object, [key UTF8String], 0);
}


- (nullable id)m_objectForKey:(NSString *)key {
    if (!key || ![key isKindOfClass:[NSString class]] ||key.length == 0) {
        return nil;
    }
    auto p = _memorycache->get([key UTF8String]);
    if (p.second == -1 || p.first == NULL) {
        return nil;
    }
    return (__bridge id)p.first;
}

- (void)m_removeAllObjects {
    _memorycache->removeAllObj();
}

- (void)d_setObject:(nullable id)object forKey:(NSString *)key {
    if (!key || key.length == 0) {
        return ;
    }
    NSData *value = [NSKeyedArchiver archivedDataWithRootObject:object];
    _diskcache->set(value.bytes, [key UTF8String], value.length);
}

- (nullable id)d_objectForKey:(NSString *)key {
    if (!key || key.length == 0) {
        return nil;
    }
    auto p = _diskcache->get([key UTF8String]);
    if (p.second != 0) {
        NSData *data = [NSData dataWithBytes:p.first length:p.second];
        id object = [NSKeyedUnarchiver unarchiveObjectWithData:data];
        return object;
    }
    return nil;
}
@end
