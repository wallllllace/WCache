//
//  WDiskCache.hpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#ifndef WDiskCache_hpp
#define WDiskCache_hpp

#include <stdio.h>
#include <string>
#include <sqlite3.h>
#include <iostream>

#define ROOTPATHLEN 500
#define TABLENAMELEN 100

namespace WDCache {

class WDiskCache {
    
public:
    WDiskCache(const char *rootPath, const char *tableName);
    ~WDiskCache(){
        _db_close();
    }
    
    void set(const void *value, const std::string key, size_t cost);
    std::pair<const void *, int> get(const std::string key);
    bool contain(const std::string key);
    void removeObj(const std::string key);
    void removeObjs(std::vector<std::string>& keys);
    void removeAllObj();

private:
    char _rootPath[ROOTPATHLEN];
    char _tableName[TABLENAMELEN];
    sqlite3 *_db = NULL;
    
    bool _db_open();
    void _db_close();
};

};


#endif /* WDiskCache_hpp */
