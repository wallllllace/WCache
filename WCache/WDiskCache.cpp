//
//  WDiskCache.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include "WDiskCache.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <fstream>

namespace WDCache {

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
        std::cout << "sqlite stmt update error" << std::endl;
        return false;
    }
    return true;
    return false;
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
        return {NULL, -1};
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
        return {NULL, -1};
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


WDiskCache::WDiskCache(const char *rootPath, const char *tableName) {
    if (strlen(rootPath) > ROOTPATHLEN) {
        throw "rootPath too long";
    }
    strcpy(_rootPath, rootPath);
    if (strlen(tableName) > TABLENAMELEN) {
        throw "tableName too long";
    }
    strcpy(_tableName, tableName);
    std::cout << "rootpath长度：" << strlen(rootPath) <<std::endl;
    if (!_db_open()) {
        _db_close();
        throw "init fail";
    }
};

// 增、改
void WDiskCache::set(const void *value, const std::string key, size_t cost) {
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
std::pair<const void *, int> WDiskCache::get(const std::string key) {
    if (!_db_open()) {
        return {NULL, -1};
    }
    return _db_get(_db, key.c_str());
}

bool WDiskCache::contain(const std::string key) {
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

void WDiskCache::removeObj(const std::string key) {
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

bool WDiskCache::_db_open() {
    FILE *f = fopen(_rootPath, "r");
    if (_db && f) {
        return true;
    } else {
        int result = sqlite3_open(_rootPath, &_db);
        if (result == SQLITE_OK) {
            std::cout << "_db创建成功" << std::endl;
            const char *sql = "CREATE TABLE IF NOT EXISTS t_WDiskCache (id integer PRIMARY KEY AUTOINCREMENT, value blob NOT NULL, key text NOT NULL, size integer NOT NULL, modification_time integer, last_access_time integer);";
            char * errorMsg;
            sqlite3_exec(_db, sql, NULL, NULL, &errorMsg);
            if (errorMsg) {
                std::cout << "表创建失败" << std::endl;
                _db_close();
            } else {
                std::cout << "表创建成功, rootPath: " << _rootPath << ", tableName: " << _tableName << std::endl;
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

};





