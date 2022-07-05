//
//  WCache.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include "WCache.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <fstream>
#include <thread>

namespace WCache {

WCache::WCache(const std::string& rootPath, const std::string& cacheName, size_t m_count_limit, size_t m_cost_limit, size_t d_count_limit, size_t d_cost_limit): _memoryCache(std::make_shared<WMemoryCache>(m_count_limit, m_cost_limit)), _diskCache(std::make_shared<WDiskCache>(rootPath.c_str(), cacheName.c_str(), d_count_limit, d_cost_limit)) {
    std::cout << "WCache init" << std::endl;
}

std::pair<const void *, int> WCache::get(const std::string& key) {
    auto p = _memoryCache->get(key);
    if (p.second == 0) {
        p = _diskCache->get(key);
        if (p.second != 0) {
            _memoryCache->set(p.first, key, p.second);
        }
    } else {
        _diskCache->update(key);
    }
    return p;
}

void WCache::set(const void *value, const std::string& key, size_t cost) {
    _memoryCache->set(value, key, cost);
    _diskCache->set(value, key, cost);
}

bool WCache::contain(const std::string& key) {
    return _memoryCache->contain(key) || _diskCache->contain(key);
}

void WCache::removeObj(const std::string& key) {
    _memoryCache->removeObj(key);
    _diskCache->removeObj(key);
}

void WCache::removeAllObj() {
    _memoryCache->removeAllObj();
    _diskCache->removeAllObj();
}


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

std::shared_ptr<CacheNode> WMemoryCache::_get(const std::string& key) {
    _mutex.lock();
    auto it = _lru->_map.find(key);
    _mutex.unlock();
    if (it == _lru->_map.cend()) {
        return nullptr;
    }
    it->second->_timestamp = (unsigned long long)time(NULL);
    return it->second;
}

std::pair<const void *, int>WMemoryCache::get(const std::string& key) {
    auto p = _get(key);
    if (p == nullptr) {
        return {NULL, 0};
    }
    _lru->bringNodeToHead(p);
    return {p->_value, p->_length};
}

void WMemoryCache::set(const void *value, const std::string& key, size_t cost) {
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

bool WMemoryCache::contain(const std::string& key) {
    _mutex.lock();
    auto it = _lru->_map.find(key);
    _mutex.unlock();
    if (it == _lru->_map.cend()) {
        return false;
    }
    return true;
}

void WMemoryCache::removeObj(const std::string& key) {
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


sqlite3_stmt * _dbPrepareStmt(sqlite3 *db, const char *sql) {
    if (!sql) {
        return NULL;
    }
    sqlite3_stmt *stmt = NULL;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (result != SQLITE_OK) {
        std::cout << "sqlite stmt prepare error" << std::endl;
        return NULL;
    } else {
        sqlite3_reset(stmt);
    }
    return stmt;
}

bool _db_save(sqlite3 *db, const char * key, const void * value, size_t size) {
    const char * sql = "insert into t_WDiskCache (value, key, size, modification_time, last_access_time) values (?1, ?2, ?3, ?4, ?5);";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
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
        std::cout << "sqlite stmt save error" << std::endl;
        return false;
    }
    return true;
}

bool _db_update(sqlite3 *db, const char * key, const void * value, size_t size) {
    const char * sql = "update t_WDiskCache set value = ?1, size = ?2, modification_time = ?3, last_access_time = ?4 where key = ?5;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
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
        std::cout << "sqlite update error" << std::endl;
        return false;
    }
    return true;
}

bool _db_updateAccessTime(sqlite3 *db, const char *key) {
    const char *sql = "update t_WDiskCache set last_access_time = ?1 where key = ?2;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return false;
    }
    int timestamp = (int)time(NULL);
    sqlite3_bind_int(stmt, 1, timestamp);
    sqlite3_bind_text(stmt, 2, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cout << "sqlite update last_access_time error" << std::endl;
        return false;
    }
    return true;
}

bool _db_delete(sqlite3 *db, const char * key) {
    const char *sql = "delete from t_WDiskCache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cout << " db delete error" << std::endl;
        return false;
    }
    return true;
}

bool _db_deleteItems(sqlite3 *db, std::vector<std::string> &keys) {
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
    std::string sql = "delete from t_WDiskCache where key in (";
    sql += s + ");";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql.c_str());
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
        std::cout << "sqlite delete items error" << std::endl;
        return false;
    }
    return true;
}


std::pair<const void *, int>_db_get(sqlite3 *db, const char *key) {
    const char *sql = "select value from t_WDiskCache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
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
            std::cout << "sqlite query error" << std::endl;
        }
        return {NULL, 0};
    }
}

void _db_getItem(sqlite3 *db, const char *key) {
    const char *sql = "select key, value, size, modification_time, last_access_time from t_WDiskCache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
        
    } else {
        if (result != SQLITE_DONE) {
            std::cout << "sqlite query error" << std::endl;
        }
        return;
    }
}

unsigned int _db_getCount(sqlite3 *db, const char *key){
    const char *sql = "select count(key) from t_WDiskCache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        std::cout << "sqlite query error" << std::endl;
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

unsigned int _db_getTotalCount(sqlite3 *db) {
    const char *sql = "select count(*) from t_WDiskCache;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return -1;
    }
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        std::cout << "sqlite query totalCount error" << std::endl;
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

unsigned int _db_getTotalCost(sqlite3 *db) {
    const char *sql = "select sum(size) from t_WDiskCache;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return -1;
    }
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        std::cout << "sqlite query totalCost error" << std::endl;
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

unsigned int _db_getTotal(sqlite3 *db, const char *key){
    const char *sql = "select sum(size) from t_WDiskCache where key = ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
    if (!stmt) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, key, -1, NULL);
    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        std::cout << "sqlite query error" << std::endl;
        return -1;
    }
    return sqlite3_column_int(stmt, 0);
}

std::vector<std::pair<const char *, int>> _db_getItemByTimeAsc(sqlite3 *db, unsigned int count) {
    const char *sql = "select key, size from t_WDiskCache order by last_access_time asc limit ?1;";
    sqlite3_stmt *stmt = _dbPrepareStmt(db, sql);
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
            std::cout << "sqlite getItemByTimeAsc error" << std::endl;
            break;
        }
    } while (1);
    return v;
}


WDiskCache::WDiskCache(const char *rootPath, const char *cacheName, size_t count, size_t cost) : _countLimit(count), _costLimit(cost) {
    if (strlen(rootPath) > ROOTPATHLEN) {
        throw "rootPath too long";
    }
    strcpy(_rootPath, rootPath);
    if (strlen(cacheName) > TABLENAMELEN) {
        throw "tableName too long";
    }
    strcpy(_cacheName, cacheName);
    std::cout << "rootpath长度：" << strlen(rootPath) <<std::endl;
    if (!_db_open()) {
        _db_close();
        throw "init fail";
    }
};

// 增、改
void WDiskCache::set(const void *value, const std::string& key, size_t cost) {
    if (!_db_open()) {
        return;
    }
    if (contain(key)) {
        bool result = _db_update(_db, key.c_str(), value, cost);
        if (result) {
            std::cout << "更新成功，key:" << key << std::endl;
        } else {
            std::cout << "更新失败，key:" << key << std::endl;
        }
        return;
    }
    bool result = _db_save(_db, key.c_str(), value, cost);
    if (result) {
        std::cout << "插入成功，key:" << key << std::endl;
    } else {
        std::cout << "插入失败，key:" << key << std::endl;
    }
}

// 查
std::pair<const void *, int> WDiskCache::get(const std::string& key) {
    if (!_db_open()) {
        return {NULL, 0};
    }
    const char *k = key.c_str();
    auto p = _db_get(_db, k);
    _db_updateAccessTime(_db, k);
    return p;
}

bool WDiskCache::contain(const std::string& key) {
    if (!_db_open()) {
        return false;
    }
    unsigned int result = _db_getCount(_db, key.c_str());
    if (result == -1) {
        std::cout << "查询数量失败，key:" << key << std::endl;
        return false;
    } else if(result > 0) {
        return true;
    }
    return false;
}

void WDiskCache::update(const std::string& key) {
    _db_updateAccessTime(_db, key.c_str());
}

void WDiskCache::removeObj(const std::string& key) {
    if (!_db_open()) {
        return ;
    }
    bool result = _db_delete(_db, key.c_str());
    if (result) {
        std::cout << "删除成功，key:" << key << std::endl;
    } else {
        std::cout << "删除失败，key:" << key << std::endl;
    }
}

void WDiskCache::removeObjs(std::vector<std::string> &keys) {
    if (!_db_open()) {
        return ;
    }
    bool result = _db_deleteItems(_db, keys);
    if (result) {
        std::cout << "批量删除成功" << std::endl;
    } else {
        std::cout << "批量删除失败" << std::endl;
    }
}

void WDiskCache::removeAllObj() {
    _db_close();
    int result = std::remove(_rootPath);
    if (result == 0) {
        std::cout << "删除数据库成功" << std::endl;
    } else {
        std::cout << "删除数据库失败" << std::endl;
    }
}

void WDiskCache::trimToFitLimit() {
    if (_countLimit > 0) {
        _trim_fit_count(_countLimit);
    }
    if (_costLimit > 0) {
        _trim_fit_cost(_costLimit);
    }
}

bool WDiskCache::_db_open() {
    FILE *f = fopen(_rootPath, "r");
    if (_db && f) {
        return true;
    } else {
        char cache_path[ROOTPATHLEN + TABLENAMELEN + 1];
        strcpy(cache_path, _rootPath);
        strcat(cache_path, "/");
        strcat(cache_path, _cacheName);
        int result = sqlite3_open(cache_path, &_db);
        if (result == SQLITE_OK) {
            std::cout << "_db创建成功" << std::endl;
            const char *sql = "CREATE TABLE IF NOT EXISTS t_WDiskCache (id integer PRIMARY KEY AUTOINCREMENT, value blob NOT NULL, key text NOT NULL, size integer NOT NULL, modification_time integer, last_access_time integer);";
            char * errorMsg;
            sqlite3_exec(_db, sql, NULL, NULL, &errorMsg);
            if (errorMsg) {
                std::cout << "表创建失败" << std::endl;
                _db_close();
            } else {
                std::cout << "表创建成功, rootPath: " << _rootPath << ", cacheName: " << _cacheName << std::endl;
                return true;
            }
        } else {
            std::cout << "_db创建失败" << std::endl;
            _db_close();
        }
    }
    return false;
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
            std::cout << "sqlite close failed" << std::endl;
        }
    } while (retry);
    _db = NULL;
}

void WDiskCache::_trim_fit_count(size_t maxCount) {
    unsigned int total = _db_getTotalCount(_db);
    if (maxCount > total) {
        return;
    }
    bool suc = false;
    do {
        unsigned int perCount = 16;
        auto v = _db_getItemByTimeAsc(_db, perCount);
        for (auto &item : v) {
            if (total > maxCount) {
                suc = _db_delete(_db, item.first);
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
    unsigned int total = _db_getTotalCost(_db);
    if (maxCost > total) {
        return;
    }
    bool suc = false;
    do {
        unsigned int perCount = 16;
        auto v = _db_getItemByTimeAsc(_db, perCount);
        for (auto &item : v) {
            if (total > maxCost) {
                suc = _db_delete(_db, item.first);
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

};


