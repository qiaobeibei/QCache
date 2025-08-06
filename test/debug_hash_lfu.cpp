#include <iostream>
#include <string>
#include <chrono>

#include "HashLfuCache.h"
#include "LfuCache.h"

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

void debugHashLfuIssues() {
    std::cout << "=== Debug Hash-LFU Issues ===" << std::endl;
    
    const int CAPACITY = 20;
    const int OPERATIONS = 10000;
    
    // 创建普通LFU和Hash-LFU进行对比
    QCache::LfuCache<int, std::string> normal_lfu(CAPACITY);
    QCache::HashLfuCache<int, std::string> hash_lfu(CAPACITY, 4); // 问题1：max_average_freq使用默认值10
    
    std::cout << "Hash-LFU capacity: " << hash_lfu.capacity() << std::endl;
    std::cout << "Hash-LFU slice count: " << hash_lfu.sliceCount() << std::endl;
    
    // 计算每个分片的实际容量
    size_t slice_size = std::ceil(CAPACITY / static_cast<double>(4));
    std::cout << "Expected slice size: " << slice_size << std::endl;
    std::cout << "Total slice capacity: " << slice_size * 4 << std::endl;
    
    int normal_hits = 0, hash_hits = 0;
    int normal_gets = 0, hash_gets = 0;
    
    Timer normal_timer, hash_timer;
    
    // 测试普通LFU
    normal_timer = Timer();
    for (int op = 0; op < OPERATIONS; ++op) {
        int key = op % 50; // 50个不同的键
        std::string value = "value" + std::to_string(key);
        
        if (op % 3 == 0) { // 33%概率写入
            normal_lfu.put(key, value);
        } else { // 67%概率读取
            std::string result;
            normal_gets++;
            if (normal_lfu.get(key, result)) {
                normal_hits++;
            }
        }
    }
    double normal_time = normal_timer.elapsed();
    
    // 测试Hash-LFU
    hash_timer = Timer();
    for (int op = 0; op < OPERATIONS; ++op) {
        int key = op % 50; // 50个不同的键
        std::string value = "value" + std::to_string(key);
        
        if (op % 3 == 0) { // 33%概率写入
            hash_lfu.put(key, value);
        } else { // 67%概率读取
            std::string result;
            hash_gets++;
            if (hash_lfu.get(key, result)) {
                hash_hits++;
            }
        }
    }
    double hash_time = hash_timer.elapsed();
    
    // 打印结果
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Normal LFU:" << std::endl;
    std::cout << "  Hit rate: " << (100.0 * normal_hits / normal_gets) << "%" << std::endl;
    std::cout << "  Time: " << normal_time << "ms" << std::endl;
    
    std::cout << "Hash-LFU:" << std::endl;
    std::cout << "  Hit rate: " << (100.0 * hash_hits / hash_gets) << "%" << std::endl;
    std::cout << "  Time: " << hash_time << "ms" << std::endl;
    
    // 测试purge方法
    std::cout << "\n=== Testing purge method ===" << std::endl;
    try {
        hash_lfu.purge(); // 这里会有问题，因为缺少括号
        std::cout << "Purge method called successfully" << std::endl;
    } catch (...) {
        std::cout << "Purge method failed!" << std::endl;
    }
}

void testMaxAverageFreqImpact() {
    std::cout << "\n=== Testing max_average_freq Impact ===" << std::endl;
    
    const int CAPACITY = 20;
    const int OPERATIONS = 5000;
    
    // 测试不同的max_average_freq值
    std::vector<int> max_freq_values = {10, 100, 1000, 10000};
    
    for (int max_freq : max_freq_values) {
        QCache::HashLfuCache<int, std::string> hash_lfu(CAPACITY, 4, max_freq);
        
        int hits = 0, gets = 0;
        Timer timer;
        
        // 创建一个有明显热点的访问模式
        for (int op = 0; op < OPERATIONS; ++op) {
            int key;
            if (op % 100 < 80) { // 80%访问热点数据
                key = op % 5; // 5个热点键
            } else { // 20%访问冷数据
                key = 5 + (op % 45); // 45个冷键
            }
            
            std::string value = "value" + std::to_string(key);
            
            if (op % 4 == 0) { // 25%概率写入
                hash_lfu.put(key, value);
            } else { // 75%概率读取
                std::string result;
                gets++;
                if (hash_lfu.get(key, result)) {
                    hits++;
                }
            }
        }
        
        double time = timer.elapsed();
        double hit_rate = gets > 0 ? (100.0 * hits / gets) : 0.0;
        
        std::cout << "max_average_freq=" << max_freq 
                  << ": hit_rate=" << hit_rate << "%, time=" << time << "ms" << std::endl;
    }
}

int main() {
    debugHashLfuIssues();
    testMaxAverageFreqImpact();
    return 0;
} 