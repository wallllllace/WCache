//
//  WCache.h
//  WCache
//
//  Created by wangxiaorui19 on 2022/7/6.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface WCache : NSObject
- (instancetype)initWithPath:(NSString *)path
                   cacheName:(NSString *)cacheName;

- (instancetype)initWithPath:(NSString *)path
                   cacheName:(NSString *)cacheName
            memoryCountLimit:(unsigned long long)memoryCountLimit
             memoryCostLimit:(unsigned long long)memoryCostLimit
              diskCountLimit:(unsigned long long)diskCountLimit
               diskCostLimit:(unsigned long long)diskCostLimit;

- (BOOL)containsObjectForKey:(NSString *)key ;

- (nullable id)objectForKey:(NSString *)key ;

- (void)setObject:(nullable id)object forKey:(NSString *)key ;

- (void)removeObjectForKey:(NSString *)key ;

- (void)removeAllObjects ;

- (void)trimBackground ;

#pragma mark - test code

- (void)m_setObject:(nullable id)object forKey:(NSString *)key ;

- (nullable id)m_objectForKey:(NSString *)key;

- (void)m_removeAllObjects;

- (void)d_setObject:(nullable id)object forKey:(NSString *)key;

- (nullable id)d_objectForKey:(NSString *)key;

@end


NS_ASSUME_NONNULL_END
