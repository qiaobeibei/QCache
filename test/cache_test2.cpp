#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <thread>

// 包含分片缓存头文件
#include "HashLruCaches.h"
#include "HashLfuCache.h"

/**
 * @brief 简单的计时器类
 */
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

/**
 * @brief 打印测试结果
 */
void printResults(const std::string& test_name, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits) {
    std::cout << "=== " << test_name << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    
    std::vector<std::string> names = {"分片LRU", "分片LFU"};
    
    for (size_t i = 0; i < hits.size() && i < names.size(); ++i) {
        double hit_rate = 100.0 * hits[i] / get_operations[i];
        std::cout << names[i] 
                  << " - 命中率: " << std::fixed << std::setprecision(2) 
                  << hit_rate << "% ";
        std::cout << "(" << hits[i] << "/" << get_operations[i] << ")" << std::endl;
    }
    
    std::cout << std::endl;
}

/**
 * @brief 测试分片缓存的并发性能
 */
void testShardedCachePerformance() {
    std::cout << "\n=== 测试场景：分片缓存性能测试 ===" << std::endl;
    
    const int kCapacity = 100;        // 总缓存容量
    const int kOperations = 100000;   // 总操作次数
    const int kSliceNum = 4;          // 分片数量
    const int kKeyRange = 1000;       // 键的范围
    
    QCache::HashLruCaches<int, std::string> sharded_lru(kCapacity, kSliceNum);
    QCache::HashLfuCache<int, std::string> sharded_lfu(kCapacity, kSliceNum);

    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::vector<int> hits(2, 0);
    std::vector<int> get_operations(2, 0);

    // 测试分片LRU
    std::cout << "测试分片LRU缓存..." << std::endl;
    Timer lru_timer;
    
    // 预热
    for (int key = 0; key < kKeyRange / 10; ++key) {
        std::string value = "value" + std::to_string(key);
        sharded_lru.put(key, value);
    }
    
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 25); 
        int key = gen() % kKeyRange;
        
        if (is_put) {
            std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
            sharded_lru.put(key, value);
        } else {
            std::string result;
            get_operations[0]++;
            if (sharded_lru.get(key, result)) {
                hits[0]++;
            }
        }
    }
    
    double lru_time = lru_timer.elapsed();
    std::cout << "分片LRU测试完成，耗时: " << lru_time << " 毫秒" << std::endl;

    // 测试分片LFU
    std::cout << "测试分片LFU缓存..." << std::endl;
    Timer lfu_timer;
    
    gen.seed(rd()); // 重置随机数生成器
    
    // 预热
    for (int key = 0; key < kKeyRange / 10; ++key) {
        std::string value = "value" + std::to_string(key);
        sharded_lfu.put(key, value);
    }
    
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 25); 
        int key = gen() % kKeyRange;
        
        if (is_put) {
            std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
            sharded_lfu.put(key, value);
        } else {
            std::string result;
            get_operations[1]++;
            if (sharded_lfu.get(key, result)) {
                hits[1]++;
            }
        }
    }
    
    double lfu_time = lfu_timer.elapsed();
    std::cout << "分片LFU测试完成，耗时: " << lfu_time << " 毫秒" << std::endl;

    printResults("分片缓存性能测试", kCapacity, get_operations, hits);
    
    std::cout << "性能对比:" << std::endl;
    std::cout << "分片LRU平均每操作耗时: " << (lru_time / kOperations) << " 毫秒" << std::endl;
    std::cout << "分片LFU平均每操作耗时: " << (lfu_time / kOperations) << " 毫秒" << std::endl;
}

/**
 * @brief 测试不同分片数量的影响
 */
void testDifferentShardCounts() {
    std::cout << "\n=== 测试场景：不同分片数量的影响 ===" << std::endl;
    
    const int kCapacity = 200;
    const int kOperations = 50000;
    const int kKeyRange = 2000;
    
    std::vector<int> shard_counts = {1, 2, 4, 8};
    
    for (int shard_count : shard_counts) {
        std::cout << "\n测试 " << shard_count << " 个分片:" << std::endl;
        
        QCache::HashLruCaches<int, std::string> sharded_lru(kCapacity, shard_count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        
        Timer timer;
        int hits = 0;
        int get_ops = 0;
        
        // 预热
        for (int key = 0; key < kKeyRange / 20; ++key) {
            std::string value = "value" + std::to_string(key);
            sharded_lru.put(key, value);
        }
        
        for (int op = 0; op < kOperations; ++op) {
            bool is_put = (gen() % 100 < 30);
            int key = gen() % kKeyRange;
            
            if (is_put) {
                std::string value = "value" + std::to_string(key);
                sharded_lru.put(key, value);
            } else {
                std::string result;
                get_ops++;
                if (sharded_lru.get(key, result)) {
                    hits++;
                }
            }
        }
        
        double elapsed_time = timer.elapsed();
        double hit_rate = 100.0 * hits / get_ops;
        
        std::cout << "  分片数: " << shard_count 
                  << ", 命中率: " << std::fixed << std::setprecision(2) << hit_rate << "%"
                  << ", 耗时: " << elapsed_time << " 毫秒"
                  << ", 平均每操作: " << (elapsed_time / kOperations) << " 毫秒" << std::endl;
    }
}

/**
 * @brief 主函数
 */
int main() {
    std::cout << "=== QCache 分片缓存测试 ===" << std::endl;
    std::cout << "硬件线程数: " << std::thread::hardware_concurrency() << std::endl;
    
    Timer total_timer;
    
    testShardedCachePerformance();
    testDifferentShardCounts();
    
    std::cout << "\n总测试时间: " << total_timer.elapsed() << " 毫秒" << std::endl;
    
    return 0;
} 