//
//  CacheTool.m
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#import "CacheTool.h"
#include "WMemoryCache.hpp"
#import <Objc/runtime.h>
//#include <cstring>
#include <cstring>

using namespace WMCache;

@implementation CacheTool{
    WMemoryCache * _memoryCache;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _memoryCache = new WMemoryCache(100, 5*1024*1024);
    }
    return self;
}
//
- (BOOL)containsObjectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
    return _memoryCache->contain(keystr);
}

- (nullable id)objectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
    auto obj = _memoryCache->get(keystr);
    if (obj == NULL) {
        return nil;
    }
//    return (__bridge id)obj;
    return (__bridge id)obj;
}

- (void)setObject:(nullable id)object forKey:(NSString *)key {
    [self setObject:object forKey:key withCost:0];
}

- (void)setObject:(nullable id)object forKey:(NSString *)key withCost:(NSUInteger)cost {
    std::string keystr([key UTF8String]);
    void *obj = (__bridge_retained void *)object;
    _memoryCache->set(obj, keystr, cost);
}

- (void)removeObjectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
//    CFBridgingRelease(_memoryCache->get(keystr));
    _memoryCache->removeObj(keystr);
}

- (void)removeAllObjects {
    _memoryCache->removeAllObj();
}

- (void)dealloc {
    delete _memoryCache;
}

@end
