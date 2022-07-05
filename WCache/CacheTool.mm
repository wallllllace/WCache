//
//  CacheTool.m
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#import "CacheTool.h"
#include "WCache.hpp"
#import <Objc/runtime.h>
//#include <cstring>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>

@implementation CacheTool{
    WCache::WCache * _cache;
}

- (instancetype)initWithPath:(NSString *)path cacheName:(NSString *)cacheName {
    self = [super init];
    if (self) {
        try {
            _cache = new WCache::WCache([path UTF8String], [cacheName UTF8String]);
        } catch (const char *msg) {
            std::cout << "exception: " << msg << std::endl;
        }
    }
    return self;
}
//
- (BOOL)containsObjectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
    return _cache->contain(keystr);
}

- (nullable id)objectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
    auto value = _cache->get(keystr);
    if (value.second == 0) {
        return nil;
    }
    NSData *data = [NSData dataWithBytes:value.first length:value.second];
    id object = [NSKeyedUnarchiver unarchiveObjectWithData:data];
    return object;
}

- (void)setObject:(nullable id)object forKey:(NSString *)key {
    NSData *value = [NSKeyedArchiver archivedDataWithRootObject:object];
    [self setObject:value forKey:key withCost:value.length];
}

- (void)setObject:(nullable id)object forKey:(NSString *)key withCost:(NSUInteger)cost {
    std::string keystr([key UTF8String]);
    _cache->set(((NSData *)object).bytes, keystr, ((NSData *)object).length);
}

- (void)removeObjectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
    _cache->removeObj(keystr);
}

- (void)removeAllObjects {
    _cache->removeAllObj();
}

- (void)dealloc {
    delete _cache;
}

@end
