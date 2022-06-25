//
//  CacheTool.m
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#import "CacheTool.h"
#include "WMemoryCache.cpp"
#import <Objc/runtime.h>
//#include <cstring>
#include <cstring>

@implementation CacheTool{
    WMemoryCache<void *> * _memoryCache;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _memoryCache = new WMemoryCache<void *>(100, 5*1024*1024);
    }
    return self;
}

- (BOOL)containsObjectForKey:(NSString *)key {
    std::string *keystr = new std::string([key UTF8String]);
    return _memoryCache->contain(*keystr);
}

- (nullable id)objectForKey:(NSString *)key {
    std::string *keystr = new std::string([key UTF8String]);
    auto obj = _memoryCache->get(*keystr);
    if (obj == nullptr) {
        return nil;
    }
    return (__bridge id)*obj;
}

- (void)setObject:(nullable id)object forKey:(NSString *)key {
    std::string *keystr = new std::string([key UTF8String]);
    void *obj = (__bridge_retained void*)object;
    _memoryCache->set(std::make_shared<void*>(obj), *keystr);
}

- (void)setObject:(nullable id)object forKey:(NSString *)key withCost:(NSUInteger)cost {
    std::string *keystr = new std::string([key UTF8String]);
    void *obj = (__bridge_retained void*)object;
    _memoryCache->set(std::make_shared<void*>(obj), *keystr, cost);
}

- (void)removeObjectForKey:(NSString *)key {
    std::string *keystr = new std::string([key UTF8String]);
    _memoryCache->removeObj(*keystr);
}

- (void)removeAllObjects {
    _memoryCache->removeAllObj();
}

@end
