//
//  CacheTool.h
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface CacheTool : NSObject
- (BOOL)containsObjectForKey:(NSString *)key ;

- (nullable id)objectForKey:(NSString *)key ;

- (void)setObject:(nullable id)object forKey:(NSString *)key ;

- (void)setObject:(nullable id)object forKey:(NSString *)key withCost:(NSUInteger)cost ;

- (void)removeObjectForKey:(NSString *)key ;

- (void)removeAllObjects ;
@end

NS_ASSUME_NONNULL_END
