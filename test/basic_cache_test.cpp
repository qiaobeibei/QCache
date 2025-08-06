#include <iostream>
#include <string>
#include <cassert>
#include <vector>

// 包含所有缓存实现的头文件
#include "QCachePolicy.h"
#include "LruCache.h"
#include "LruKCache.h"
#include "HashLruCaches.h"
#include "LfuCache.h"
#include "HashLfuCache.h"
#include "ArcCache.h"

/**
 * @brief 基本功能测试
 */
void testBasicFunctionality() {
    std::cout << "=== 基本功能测试 ===" << std::endl;
    
    // 测试LRU缓存
    {
        std::cout << "测试LRU缓存..." << std::endl;
        QCache::LruCache<int, std::string> lru(3);
        
        // 测试put和get
        lru.put(1, "value1");
        lru.put(2, "value2");
        lru.put(3, "value3");
        
        std::string value;
        assert(lru.get(1, value) && value == "value1");
        assert(lru.get(2, value) && value == "value2");
        assert(lru.get(3, value) && value == "value3");
        
        // 测试容量限制
        lru.put(4, "value4"); // 应该淘汰最久未使用的
        assert(!lru.get(1, value)); // value1应该被淘汰
        assert(lru.get(4, value) && value == "value4");
        
        std::cout << "LRU缓存测试通过!" << std::endl;
    }
    
    // 测试LFU缓存
    {
        std::cout << "测试LFU缓存..." << std::endl;
        QCache::LfuCache<int, std::string> lfu(3);
        
        lfu.put(1, "value1");
        lfu.put(2, "value2");
        lfu.put(3, "value3");
        
        std::string value;
        // 增加访问频率
        lfu.get(1, value);
        lfu.get(1, value);
        lfu.get(2, value);
        
        // 添加新元素，应该淘汰频率最低的
        lfu.put(4, "value4");
        
        assert(lfu.get(1, value) && value == "value1"); // 频率高，应该保留
        assert(lfu.get(4, value) && value == "value4");
        
        std::cout << "LFU缓存测试通过!" << std::endl;
    }
    
    // 测试ARC缓存
    {
        std::cout << "测试ARC缓存..." << std::endl;
        QCache::ArcCache<int, std::string> arc(5);
        
        arc.put(1, "value1");
        arc.put(2, "value2");
        arc.put(3, "value3");
        
        std::string value;
        assert(arc.get(1, value) && value == "value1");
        assert(arc.get(2, value) && value == "value2");
        assert(arc.get(3, value) && value == "value3");
        
        std::cout << "ARC缓存测试通过!" << std::endl;
    }
    
    // 测试LRU-K缓存
    {
        std::cout << "测试LRU-K缓存..." << std::endl;
        QCache::LruKCache<int, std::string> lruk(3, 10, 2);
        
        lruk.put(1, "value1");
        lruk.put(2, "value2");
        
        std::string value;
        // 第一次访问，应该在历史队列中
        assert(lruk.get(1, value) && value == "value1");
        // 第二次访问，应该进入缓存队列
        assert(lruk.get(1, value) && value == "value1");
        
        std::cout << "LRU-K缓存测试通过!" << std::endl;
    }
    
    // 测试分片LRU缓存
    {
        std::cout << "测试分片LRU缓存..." << std::endl;
        QCache::HashLruCaches<int, std::string> hashLru(6, 2); // 总容量6，2个分片
        
        hashLru.put(1, "value1");
        hashLru.put(2, "value2");
        hashLru.put(3, "value3");
        
        std::string value;
        assert(hashLru.get(1, value) && value == "value1");
        assert(hashLru.get(2, value) && value == "value2");
        assert(hashLru.get(3, value) && value == "value3");
        
        std::cout << "分片LRU缓存测试通过!" << std::endl;
    }
    
    // 测试分片LFU缓存
    {
        std::cout << "测试分片LFU缓存..." << std::endl;
        QCache::HashLfuCache<int, std::string> hashLfu(6, 2); // 总容量6，2个分片
        
        hashLfu.put(1, "value1");
        hashLfu.put(2, "value2");
        hashLfu.put(3, "value3");
        
        std::string value;
        assert(hashLfu.get(1, value) && value == "value1");
        assert(hashLfu.get(2, value) && value == "value2");
        assert(hashLfu.get(3, value) && value == "value3");
        
        std::cout << "分片LFU缓存测试通过!" << std::endl;
    }
    
    std::cout << "所有基本功能测试通过!" << std::endl;
}

int main() {
    try {
        testBasicFunctionality();
        std::cout << "\n所有测试完成!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "未知错误" << std::endl;
        return 1;
    }
    
    return 0;
} 