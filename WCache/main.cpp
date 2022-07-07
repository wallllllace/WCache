//
//  main.cpp
//  WCache
//
//  Created by wangxiaorui19 on 2022/6/25.
//

#include <iostream>
#include <thread>
#include <string>
#include <iostream>
#include <memory>
//#include "WCache.hpp"

//using namespace WCache;

//void func1(WMemoryCache<int> *cache, const std::string key, std::shared_ptr<int> value) {
//    std::cout << "func1的key：" << key << " ,func1的value：" << *value << std::endl;
//    cache->set(value, key);
//}
//
//void func2(WMemoryCache<int> *cache, const std::string key) {
//    std::cout << "func2的key：" << key << std::endl;
////    auto obj = cache->get(key);
////    if (obj != nullptr) {
////        int i = *obj;
////        std::cout << "读取cache数据：" << i << std::endl;
////    }
//    cache->removeObj(key);
//}

struct Item {
public:
    Item(int v): value(v){}
    int value;
};

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    
//    WCache *cache = new WMemoryCache(100, 3*1024*1024);
//    Item *i1 = new Item(1);
//    cache->set(i1, std::to_string(1), sizeof(*i1));
//    Item *i2 = new Item(2);
//    cache->set(i2, std::to_string(2), sizeof(*i2));
//    Item *ii2 = static_cast<Item *>(cache->get(std::to_string(2)));
//    std::cout << "ii2: " << ii2->value << std::endl;
//    Item *ii1 = static_cast<Item *>(cache->get(std::to_string(1)));
//    std::cout << "ii1: " << ii1->value << std::endl;
//
//    std::thread ths[1000];
//    for (int i = 0; i<1000; ++i) {
//        ths[i] = std::thread(func1, cache, std::to_string(i), std::make_shared<int>(i));
//    }
//    std::thread ths2[1000];
//    for (int i = 0; i<1000; ++i) {
//        ths2[i] = std::thread(func2, cache, std::to_string(i));
//    }
//
//    std::cout << "主线程执行开始..." << std::endl;
//
//    for (auto& th : ths) {
//        th.join();
//    }
//    for (auto& th : ths2) {
//        th.join();
//    }
//
////    th1.join();
////    th2.join();
////
////    std::cout << "主线程取数据执行开始..." << std::endl;
////    for (int i = 1999; i>0; --i) {
////        auto obj = cache->get(std::to_string(i));
////        int *j = (int *)obj;
////        std::cout << *j << std::endl;
////    }
//    std::cout << "主线程执行结束..." << std::endl;
//    delete i1;
//    delete i2;
//    delete cache;
    return 0;
}
