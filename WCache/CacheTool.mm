//
//  CacheTool.m
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#import "CacheTool.h"
#include "WMemoryCache.hpp"
#include "WDiskCache.hpp"
#import <Objc/runtime.h>
//#include <cstring>
#include <cstring>
#include <vector>
#include <string>

using namespace WMCache;
using namespace WDCache;

@implementation CacheTool{
    WMemoryCache * _memoryCache;
    WDiskCache * _diskCache;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _memoryCache = new WMemoryCache(2, 5*1024*1024);
        NSString *testPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
        NSString *rootPath = [testPath stringByAppendingPathComponent:@"WCachePrivate"];
        _diskCache = new WDiskCache([rootPath UTF8String], [@"WcacheTable" UTF8String]);
        
        
        /*
//        std::string *s = new std::string("aaa");
        char *s = (char *)malloc(sizeof(char) * 3);
        const char *c_s = "aaa";
        memcpy(s, c_s, sizeof(char) * 3);
        NSString *s_objc = [[NSString alloc] initWithUTF8String:s];
        NSHashTable *hashTable = [NSHashTable weakObjectsHashTable];
        [hashTable addObject:s_objc];
        if ([hashTable containsObject:s_objc]) {
            NSLog(@"存在，s_objc：%@",s_objc);
            std::cout << "存在，s:" << *s << std::endl;
        } else {
            NSLog(@"不存在");
        }
        free(s);
        if ([hashTable containsObject:s_objc]) {
            NSLog(@"存在，s_objc：%@",s_objc);
            std::cout << "存在，s:" << s << std::endl;
        } else {
            NSLog(@"不存在");
        }
         */
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
    auto value = _memoryCache->get(keystr);
    if (value.second == 0) {
        return nil;
    }
    NSData *data = [NSData dataWithBytes:value.first length:value.second];
    id object = [NSKeyedUnarchiver unarchiveObjectWithData:data];
    return object;
}

- (nullable id)objectFromDiskForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
    auto value = _diskCache->get(keystr);
    if (value.second == 0) {
        return nil;
    }
    NSData *data = [NSData dataWithBytes:value.first length:value.second];
//    id object = [NSKeyedUnarchiver unarchiveObjectWithData:(__bridge NSData *)value.first];
    NSError *err;
//    NSData *data = (__bridge NSData *)value.first;
//    id object = [NSKeyedUnarchiver unarchiveTopLevelObjectWithData:data error:&err];
    id object = [NSKeyedUnarchiver unarchiveObjectWithData:data];
    return object;
}

- (void)setObject:(nullable id)object forKey:(NSString *)key {
    NSData *value = [NSKeyedArchiver archivedDataWithRootObject:object];
    [self setObject:value forKey:key withCost:value.length];
}

- (void)setObject:(nullable id)object forKey:(NSString *)key withCost:(NSUInteger)cost {
    std::string keystr([key UTF8String]);
//    void *obj = (__bridge_retained void *)object;
    _memoryCache->set(((NSData *)object).bytes, keystr, ((NSData *)object).length);
    _diskCache->set(((NSData *)object).bytes, keystr, ((NSData *)object).length);
}

- (void)removeObjectForKey:(NSString *)key {
    std::string keystr([key UTF8String]);
//    CFBridgingRelease(_memoryCache->get(keystr));
    _memoryCache->removeObj(keystr);
    _diskCache->removeObj(keystr);
}

- (void)removeObjectForKeys:(NSArray<NSString *> *)keys {
    std::vector<std::string> ckeys;
    for (int i = 0; i < keys.count; ++i) {
        ckeys.push_back(static_cast<std::string>([keys[i] UTF8String]));
    }
    _diskCache->removeObjs(ckeys);
}

- (void)removeAllObjects {
    _memoryCache->removeAllObj();
}

- (void)dealloc {
    delete _memoryCache;
    delete _diskCache;
}

@end
