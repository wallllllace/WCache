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

namespace WDCache {

class WDiskCache {
    
public:
    WDiskCache(const char *rootPath, const char *tableName);
    
private:
    const char * _rootPath;
    const char * _tableName;
    sqlite3 *_db = NULL;
};

};


#endif /* WDiskCache_hpp */
