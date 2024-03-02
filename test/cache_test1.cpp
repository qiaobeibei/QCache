#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <array>

// 包含缓存头文件（现在可以直接包含，不需要相对路径）
#include "QCachePolicy.h"
#include "LfuCache.h"
#include "LruCache.h"
#include "LruKCache.h"

/**
 * @brief 简单的计时器类
 */
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    /**
     * @brief 获取经过的毫秒数
     * 
     * @return double 经过的毫秒数
     */
    double elapsed() {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

/**
 * @brief 打印测试结果
 * 
 * @param test_name 测试名称
 * @param capacity 缓存容量
 * @param get_operations 各算法的获取操作次数
 * @param hits 各算法的命中次数
 */
void printResults(const std::string& test_name, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits) {
    std::cout << "=== " << test_name << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    
    std::vector<std::string> names = {"LRU", "LFU", "LRU-K"};
    
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
 * @brief 测试场景1：热点数据访问测试
 */
void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;
    
    const int kCapacity = 20;         // 缓存容量
    const int kOperations = 50000;    // 总操作次数
    const int kHotKeys = 20;         // 热点数据数量
    const int kColdKeys = 500;       // 冷数据数量
    
    QCache::LruCache<int, std::string> lru(kCapacity);
    QCache::LfuCache<int, std::string> lfu(kCapacity);
    QCache::LruKCache<int, std::string> lruk(kCapacity, kHotKeys + kColdKeys, 2);

    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 注意：由于接口问题，这里直接使用具体类型而不是基类指针
    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);

    // 为LRU缓存进行测试
    std::cout << "测试LRU缓存..." << std::endl;
    for (int key = 0; key < kHotKeys; ++key) {
        std::string value = "value" + std::to_string(key);
        lru.put(key, value);
    }
    
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 30); 
        int key;
        
        if (gen() % 100 < 70) {
            key = gen() % kHotKeys;
        } else {
            key = kHotKeys + (gen() % kColdKeys);
        }
        
        if (is_put) {
            std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
            lru.put(key, value);
        } else {
            std::string result;
            get_operations[0]++;
            if (lru.get(key, result)) {
                hits[0]++;
            }
        }
    }

    // 为LFU缓存进行测试
    std::cout << "测试LFU缓存..." << std::endl;
    for (int key = 0; key < kHotKeys; ++key) {
        std::string value = "value" + std::to_string(key);
        lfu.put(key, value);
    }
    
    gen.seed(rd()); // 重置随机数生成器以保证相同的测试序列
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 30); 
        int key;
        
        if (gen() % 100 < 70) {
            key = gen() % kHotKeys;
        } else {
            key = kHotKeys + (gen() % kColdKeys);
        }
        
        if (is_put) {
            std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
            lfu.put(key, value);
        } else {
            std::string result;
            get_operations[1]++;
            if (lfu.get(key, result)) {
                hits[1]++;
            }
        }
    }

    // 为LRU-K缓存进行测试
    std::cout << "测试LRU-K缓存..." << std::endl;
    for (int key = 0; key < kHotKeys; ++key) {
        std::string value = "value" + std::to_string(key);
        lruk.put(key, value);
    }
    
    gen.seed(rd()); // 重置随机数生成器
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 30); 
        int key;
        
        if (gen() % 100 < 70) {
            key = gen() % kHotKeys;
        } else {
            key = kHotKeys + (gen() % kColdKeys);
        }
        
        if (is_put) {
            std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
            lruk.put(key, value);
        } else {
            std::string result;
            get_operations[2]++;
            if (lruk.get(key, result)) {
                hits[2]++;
            }
        }
    }

    printResults("热点数据访问测试", kCapacity, get_operations, hits);
}

/**
 * @brief 测试场景2：循环扫描测试
 */
void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;
    
    const int kCapacity = 50;          // 缓存容量
    const int kLoopSize = 100;         // 循环范围大小
    const int kOperations = 20000;     // 总操作次数
    
    QCache::LruCache<int, std::string> lru(kCapacity);
    QCache::LfuCache<int, std::string> lfu(kCapacity);
    QCache::LruKCache<int, std::string> lruk(kCapacity, kLoopSize * 2, 2);

    std::vector<int> hits(3, 0);
    std::vector<int> get_operations(3, 0);
    std::random_device rd;
    std::mt19937 gen(rd());

    // 测试LRU
    std::cout << "测试LRU循环扫描..." << std::endl;
    for (int key = 0; key < kLoopSize / 5; ++key) {
        std::string value = "loop" + std::to_string(key);
        lru.put(key, value);
    }
    
    int current_pos = 0;
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 20);
        int key;
        
        if (op % 100 < 60) {
            key = current_pos;
            current_pos = (current_pos + 1) % kLoopSize;
        } else if (op % 100 < 90) {
            key = gen() % kLoopSize;
        } else {
            key = kLoopSize + (gen() % kLoopSize);
        }
        
        if (is_put) {
            std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
            lru.put(key, value);
        } else {
            std::string result;
            get_operations[0]++;
            if (lru.get(key, result)) {
                hits[0]++;
            }
        }
    }

    // 测试LFU（重置状态）
    std::cout << "测试LFU循环扫描..." << std::endl;
    for (int key = 0; key < kLoopSize / 5; ++key) {
        std::string value = "loop" + std::to_string(key);
        lfu.put(key, value);
    }
    
    gen.seed(rd());
    current_pos = 0;
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 20);
        int key;
        
        if (op % 100 < 60) {
            key = current_pos;
            current_pos = (current_pos + 1) % kLoopSize;
        } else if (op % 100 < 90) {
            key = gen() % kLoopSize;
        } else {
            key = kLoopSize + (gen() % kLoopSize);
        }
        
        if (is_put) {
            std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
            lfu.put(key, value);
        } else {
            std::string result;
            get_operations[1]++;
            if (lfu.get(key, result)) {
                hits[1]++;
            }
        }
    }

    // 测试LRU-K
    std::cout << "测试LRU-K循环扫描..." << std::endl;
    for (int key = 0; key < kLoopSize / 5; ++key) {
        std::string value = "loop" + std::to_string(key);
        lruk.put(key, value);
    }
    
    gen.seed(rd());
    current_pos = 0;
    for (int op = 0; op < kOperations; ++op) {
        bool is_put = (gen() % 100 < 20);
        int key;
        
        if (op % 100 < 60) {
            key = current_pos;
            current_pos = (current_pos + 1) % kLoopSize;
        } else if (op % 100 < 90) {
            key = gen() % kLoopSize;
        } else {
            key = kLoopSize + (gen() % kLoopSize);
        }
        
        if (is_put) {
            std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
            lruk.put(key, value);
        } else {
            std::string result;
            get_operations[2]++;
            if (lruk.get(key, result)) {
                hits[2]++;
            }
        }
    }

    printResults("循环扫描测试", kCapacity, get_operations, hits);
}

/**
 * @brief 主函数
 * 
 * @return int 程序退出码
 */
int main() {
    std::cout << "=== QCache 基本缓存算法测试 ===" << std::endl;
    
    Timer timer;
    
    testHotDataAccess();
    testLoopPattern();
    
    std::cout << "总测试时间: " << timer.elapsed() << " 毫秒" << std::endl;
    
    return 0;
} 