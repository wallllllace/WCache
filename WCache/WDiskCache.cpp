//
//  WDiskCache.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include "WDiskCache.hpp"
#include <iostream>


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
    const char * sql = "insert or replace into t_WDiskCache (value, key, size, modification_time, last_access_time) values (?1, ?2, ?3, ?4, ?5);";
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
        std::cout << "sqlite stmt prepare error" << std::endl;
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



//void _exec(sqlite3 *db, const char * sql) {
//    const char *sqlSentence = "INSERT INTO t_person(name, age) VALUES('夏明', 22); ";        //SQL语句
//        sqlite3_stmt *stmt = NULL;        //stmt语句句柄
//
//        //进行插入前的准备工作——检查语句合法性
//        //-1代表系统会自动计算SQL语句的长度
//        int result = sqlite3_prepare(sql, sqlSentence, -1, &stmt, NULL);
//
//        if (result == SQLITE_OK) {
//            std::clog<< "添加数据语句OK";
//            //执行该语句
//            sqlite3_step(stmt);
//        }
//        else {
//            std::clog << "添加数据语句有问题";
//        }
//        //清理语句句柄，准备执行下一个语句
//        sqlite3_finalize(stmt);
//}

WDiskCache::WDiskCache(const char *rootPath, const char *tableName): _rootPath(rootPath), _tableName(tableName) {
    if (!_db) {
        int result = sqlite3_open(_rootPath, &_db);
        if (result == SQLITE_OK) {
            std::cout << "_db创建成功" << std::endl;
            const char *sql = "CREATE TABLE IF NOT EXISTS t_WDiskCache (id integer PRIMARY KEY AUTOINCREMENT, value blob NOT NULL, key text NOT NULL, size integer NOT NULL, modification_time integer, last_access_time integer);";
            char * errorMsg;
            sqlite3_exec(_db, sql, NULL, NULL, &errorMsg);
            if (errorMsg) {
                std::cout << "表创建失败" << std::endl;
            } else {
                std::cout << "表创建成功" << std::endl;
            }
        } else {
            std::cout << "_db创建失败" << std::endl;
        }
    }
};

void WDiskCache::set(const void *value, const std::string key, size_t cost) {
    bool result = _db_save(_db, key.c_str(), value, cost);
    if (result) {
        std::cout << "插入成功，key:" << key << std::endl;
    } else {
        std::cout << "插入失败，key:" << key << std::endl;
    }
}


std::pair<const void *, int> WDiskCache::get(const std::string key) {
    return _db_get(_db, key.c_str());
}

};





