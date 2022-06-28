//
//  WDiskCache.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include "WDiskCache.hpp"


namespace WDCache {
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
}

};


