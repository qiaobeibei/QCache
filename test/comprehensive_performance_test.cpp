#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include <thread>
#include <atomic>
#include <future>

#include "QCachePolicy.h"
#include "LfuCache.h"
#include "LruCache.h"
#include "LruKCache.h"
#include "HashLruCaches.h"
#include "HashLfuCache.h"
#include "ArcCache.h"

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

// 辅助函数：打印结果
void printResults(const std::string& testName, int capacity, 
                 const std::vector<int>& get_operations, 
                 const std::vector<int>& hits,
                 const std::vector<double>& times) {
    std::cout << "=== " << testName << " 结果汇总 ===" << std::endl;
    std::cout << "缓存大小: " << capacity << std::endl;
    
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "Hash-LRU", "Hash-LFU"};
    
    std::cout << std::left << std::setw(12) << "算法" 
              << std::setw(10) << "命中率%" 
              << std::setw(15) << "命中次数/总次数"
              << std::setw(12) << "执行时间(ms)"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (size_t i = 0; i < hits.size() && i < names.size(); ++i) {
        double hitRate = get_operations[i] > 0 ? 100.0 * hits[i] / get_operations[i] : 0.0;
        std::cout << std::left << std::setw(12) << names[i]
                  << std::setw(10) << std::fixed << std::setprecision(2) << hitRate
                  << std::setw(15) << (std::to_string(hits[i]) + "/" + std::to_string(get_operations[i]))
                  << std::setw(12) << std::fixed << std::setprecision(1) << times[i]
                  << std::endl;
    }
    
    std::cout << std::endl;
}

void testHotDataAccess() {
    std::cout << "\n=== 测试场景1：热点数据访问测试 ===" << std::endl;
    
    const int CAPACITY = 50;         // 缓存容量
    const int OPERATIONS = 100000;   // 总操作次数
    const int HOT_KEYS = 30;         // 热点数据数量
    const int COLD_KEYS = 2000;      // 冷数据数量
    
    QCache::LruCache<int, std::string> lru(CAPACITY);
    QCache::LfuCache<int, std::string> lfu(CAPACITY);
    QCache::ArcCache<int, std::string> arc(CAPACITY);
    QCache::LruKCache<int, std::string> lruk(CAPACITY, HOT_KEYS + COLD_KEYS, 2);
    QCache::HashLruCaches<int, std::string> hash_lru(CAPACITY, 4);
    QCache::HashLfuCache<int, std::string> hash_lfu(CAPACITY, 4);

    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 基类指针指向派生类对象
    std::array<QCache::QCachePolicy<int, std::string>*, 4> caches = {&lru, &lfu, &arc, &lruk};
    std::vector<int> hits(6, 0);
    std::vector<int> get_operations(6, 0);
    std::vector<double> execution_times(6, 0.0);
    std::vector<std::string> names = {"LRU", "LFU", "ARC", "LRU-K", "Hash-LRU", "Hash-LFU"};

    // 为所有的缓存对象进行相同的操作序列测试
    for (int i = 0; i < 4; ++i) {
        Timer timer;
        
        // 先预热缓存，插入一些数据
        for (int key = 0; key < HOT_KEYS / 2; ++key) {
            std::string value = "value" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 重置随机数生成器以确保每个算法测试相同的序列
        gen.seed(12345);
        
        // 交替进行put和get操作，模拟真实场景
        for (int op = 0; op < OPERATIONS; ++op) {
            // 大多数缓存系统中读操作比写操作频繁
            // 所以设置25%概率进行写操作
            bool isPut = (gen() % 100 < 25); 
            int key;
            
            // 80%概率访问热点数据，20%概率访问冷数据
            if (gen() % 100 < 80) {
                key = gen() % HOT_KEYS; // 热点数据
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS); // 冷数据
            }
            
            if (isPut) {
                // 执行put操作
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i]->put(key, value);
            } else {
                // 执行get操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
        
        execution_times[i] = timer.elapsed();
    }
    
    // 测试分片缓存（接口不同，需要单独测试）
    // 测试Hash-LRU
    {
        Timer timer;
        gen.seed(12345);
        
        // 预热
        for (int key = 0; key < HOT_KEYS / 2; ++key) {
            std::string value = "value" + std::to_string(key);
            hash_lru.put(key, value);
        }
        
        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen() % 100 < 25);
            int key;
            
            if (gen() % 100 < 80) {
                key = gen() % HOT_KEYS;
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            
            if (isPut) {
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                hash_lru.put(key, value);
            } else {
                std::string result;
                get_operations[4]++;
                if (hash_lru.get(key, result)) {
                    hits[4]++;
                }
            }
        }
        
        execution_times[4] = timer.elapsed();
    }
    
    // 测试Hash-LFU
    {
        Timer timer;
        gen.seed(12345);
        
        // 预热
        for (int key = 0; key < HOT_KEYS / 2; ++key) {
            std::string value = "value" + std::to_string(key);
            hash_lfu.put(key, value);
        }
        
        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen() % 100 < 25);
            int key;
            
            if (gen() % 100 < 80) {
                key = gen() % HOT_KEYS;
            } else {
                key = HOT_KEYS + (gen() % COLD_KEYS);
            }
            
            if (isPut) {
                std::string value = "value" + std::to_string(key) + "_v" + std::to_string(op % 100);
                hash_lfu.put(key, value);
            } else {
                std::string result;
                get_operations[5]++;
                if (hash_lfu.get(key, result)) {
                    hits[5]++;
                }
            }
        }
        
        execution_times[5] = timer.elapsed();
    }

    // 打印测试结果
    printResults("热点数据访问测试", CAPACITY, get_operations, hits, execution_times);
}

void testLoopPattern() {
    std::cout << "\n=== 测试场景2：循环扫描测试 ===" << std::endl;
    
    const int CAPACITY = 100;          // 缓存容量
    const int LOOP_SIZE = 800;        // 循环范围大小
    const int OPERATIONS = 80000;    // 总操作次数
    
    QCache::LruCache<int, std::string> lru(CAPACITY);
    QCache::LfuCache<int, std::string> lfu(CAPACITY);
    QCache::ArcCache<int, std::string> arc(CAPACITY);
    QCache::LruKCache<int, std::string> lruk(CAPACITY, LOOP_SIZE, 2);
    QCache::HashLruCaches<int, std::string> hash_lru(CAPACITY, 4);
    QCache::HashLfuCache<int, std::string> hash_lfu(CAPACITY, 4);

    std::array<QCache::QCachePolicy<int, std::string>*, 4> caches = {&lru, &lfu, &arc, &lruk};
    std::vector<int> hits(6, 0);
    std::vector<int> get_operations(6, 0);
    std::vector<double> execution_times(6, 0.0);

    std::random_device rd;
    std::mt19937 gen(rd());

    // 为每种缓存算法运行相同的测试
    for (int i = 0; i < 4; ++i) {
        Timer timer;
        gen.seed(54321);
        
        // 先预热一部分数据
        for (int key = 0; key < CAPACITY / 4; ++key) {
            std::string value = "loop" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 设置循环扫描的当前位置
        int current_pos = 0;
        
        // 交替进行读写操作，模拟真实场景
        for (int op = 0; op < OPERATIONS; ++op) {
            // 15%概率是写操作，85%概率是读操作
            bool isPut = (gen() % 100 < 15);
            int key;
            
            // 按照不同模式选择键
            if (op % 100 < 70) {  // 70%顺序扫描
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else if (op % 100 < 90) {  // 20%随机跳跃
                key = gen() % LOOP_SIZE;
            } else {  // 10%访问范围外数据
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }
            
            if (isPut) {
                // 执行put操作，更新数据
                std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
                caches[i]->put(key, value);
            } else {
                // 执行get操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
        
        execution_times[i] = timer.elapsed();
    }
    
    // 测试分片缓存
    // Hash-LRU
    {
        Timer timer;
        gen.seed(54321);
        int current_pos = 0;
        
        for (int key = 0; key < CAPACITY / 4; ++key) {
            std::string value = "loop" + std::to_string(key);
            hash_lru.put(key, value);
        }
        
        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen() % 100 < 15);
            int key;
            
            if (op % 100 < 70) {
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else if (op % 100 < 90) {
                key = gen() % LOOP_SIZE;
            } else {
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }
            
            if (isPut) {
                std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
                hash_lru.put(key, value);
            } else {
                std::string result;
                get_operations[4]++;
                if (hash_lru.get(key, result)) {
                    hits[4]++;
                }
            }
        }
        
        execution_times[4] = timer.elapsed();
    }
    
    // Hash-LFU
    {
        Timer timer;
        gen.seed(54321);
        int current_pos = 0;
        
        for (int key = 0; key < CAPACITY / 4; ++key) {
            std::string value = "loop" + std::to_string(key);
            hash_lfu.put(key, value);
        }
        
        for (int op = 0; op < OPERATIONS; ++op) {
            bool isPut = (gen() % 100 < 15);
            int key;
            
            if (op % 100 < 70) {
                key = current_pos;
                current_pos = (current_pos + 1) % LOOP_SIZE;
            } else if (op % 100 < 90) {
                key = gen() % LOOP_SIZE;
            } else {
                key = LOOP_SIZE + (gen() % LOOP_SIZE);
            }
            
            if (isPut) {
                std::string value = "loop" + std::to_string(key) + "_v" + std::to_string(op % 100);
                hash_lfu.put(key, value);
            } else {
                std::string result;
                get_operations[5]++;
                if (hash_lfu.get(key, result)) {
                    hits[5]++;
                }
            }
        }
        
        execution_times[5] = timer.elapsed();
    }

    printResults("循环扫描测试", CAPACITY, get_operations, hits, execution_times);
}

void testWorkloadShift() {
    std::cout << "\n=== 测试场景3：工作负载剧烈变化测试 ===" << std::endl;
    
    const int CAPACITY = 80;            // 缓存容量
    const int OPERATIONS = 60000;       // 总操作次数
    const int PHASE_LENGTH = OPERATIONS / 6;  // 每个阶段的长度
    
    QCache::LruCache<int, std::string> lru(CAPACITY);
    QCache::LfuCache<int, std::string> lfu(CAPACITY);
    QCache::ArcCache<int, std::string> arc(CAPACITY);
    QCache::LruKCache<int, std::string> lruk(CAPACITY, 1000, 2);
    QCache::HashLruCaches<int, std::string> hash_lru(CAPACITY, 4);
    QCache::HashLfuCache<int, std::string> hash_lfu(CAPACITY, 4);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::array<QCache::QCachePolicy<int, std::string>*, 4> caches = {&lru, &lfu, &arc, &lruk};
    std::vector<int> hits(6, 0);
    std::vector<int> get_operations(6, 0);
    std::vector<double> execution_times(6, 0.0);

    // 为每种缓存算法运行相同的测试
    for (int i = 0; i < 4; ++i) { 
        Timer timer;
        gen.seed(98765);
        
        // 先预热缓存，只插入少量初始数据
        for (int key = 0; key < 20; ++key) {
            std::string value = "init" + std::to_string(key);
            caches[i]->put(key, value);
        }
        
        // 进行多阶段测试，每个阶段有不同的访问模式
        for (int op = 0; op < OPERATIONS; ++op) {
            // 确定当前阶段
            int phase = op / PHASE_LENGTH;
            
            // 每个阶段的读写比例不同 
            int putProbability;
            switch (phase) {
                case 0: putProbability = 20; break;  // 阶段1: 热点访问
                case 1: putProbability = 35; break;  // 阶段2: 大范围随机
                case 2: putProbability = 10; break;  // 阶段3: 顺序扫描
                case 3: putProbability = 30; break;  // 阶段4: 局部性随机
                case 4: putProbability = 25; break;  // 阶段5: 混合访问
                case 5: putProbability = 15; break;  // 阶段6: 回到热点
                default: putProbability = 20;
            }
            
            // 确定是读还是写操作
            bool isPut = (gen() % 100 < putProbability);
            
            // 根据不同阶段选择不同的访问模式生成key
            int key;
            if (op < PHASE_LENGTH) {  // 阶段1: 热点访问
                key = gen() % 10;
            } else if (op < PHASE_LENGTH * 2) {  // 阶段2: 大范围随机
                key = gen() % 500;
            } else if (op < PHASE_LENGTH * 3) {  // 阶段3: 顺序扫描
                key = (op - PHASE_LENGTH * 2) % 200;
            } else if (op < PHASE_LENGTH * 4) {  // 阶段4: 局部性随机
                int locality = (op / 1000) % 8;
                key = locality * 25 + (gen() % 25);
            } else if (op < PHASE_LENGTH * 5) {  // 阶段5: 混合访问
                int r = gen() % 100;
                if (r < 40) {
                    key = gen() % 10;  // 热点
                } else if (r < 70) {
                    key = 10 + (gen() % 90);  // 中等范围
                } else {
                    key = 100 + (gen() % 400);  // 大范围
                }
            } else {  // 阶段6: 回到热点访问
                key = gen() % 15;
            }
            
            if (isPut) {
                // 执行写操作
                std::string value = "value" + std::to_string(key) + "_p" + std::to_string(phase);
                caches[i]->put(key, value);
            } else {
                // 执行读操作并记录命中情况
                std::string result;
                get_operations[i]++;
                if (caches[i]->get(key, result)) {
                    hits[i]++;
                }
            }
        }
        
        execution_times[i] = timer.elapsed();
    }
    
    // 测试分片缓存
    // Hash-LRU
    {
        Timer timer;
        gen.seed(98765);
        
        for (int key = 0; key < 20; ++key) {
            std::string value = "init" + std::to_string(key);
            hash_lru.put(key, value);
        }
        
        for (int op = 0; op < OPERATIONS; ++op) {
            int phase = op / PHASE_LENGTH;
            int putProbability;
            switch (phase) {
                case 0: putProbability = 20; break;
                case 1: putProbability = 35; break;
                case 2: putProbability = 10; break;
                case 3: putProbability = 30; break;
                case 4: putProbability = 25; break;
                case 5: putProbability = 15; break;
                default: putProbability = 20;
            }
            
            bool isPut = (gen() % 100 < putProbability);
            int key;
            
            if (op < PHASE_LENGTH) {
                key = gen() % 10;
            } else if (op < PHASE_LENGTH * 2) {
                key = gen() % 500;
            } else if (op < PHASE_LENGTH * 3) {
                key = (op - PHASE_LENGTH * 2) % 200;
            } else if (op < PHASE_LENGTH * 4) {
                int locality = (op / 1000) % 8;
                key = locality * 25 + (gen() % 25);
            } else if (op < PHASE_LENGTH * 5) {
                int r = gen() % 100;
                if (r < 40) {
                    key = gen() % 10;
                } else if (r < 70) {
                    key = 10 + (gen() % 90);
                } else {
                    key = 100 + (gen() % 400);
                }
            } else {
                key = gen() % 15;
            }
            
            if (isPut) {
                std::string value = "value" + std::to_string(key) + "_p" + std::to_string(phase);
                hash_lru.put(key, value);
            } else {
                std::string result;
                get_operations[4]++;
                if (hash_lru.get(key, result)) {
                    hits[4]++;
                }
            }
        }
        
        execution_times[4] = timer.elapsed();
    }
    
    // Hash-LFU
    {
        Timer timer;
        gen.seed(98765);
        
        for (int key = 0; key < 20; ++key) {
            std::string value = "init" + std::to_string(key);
            hash_lfu.put(key, value);
        }
        
        for (int op = 0; op < OPERATIONS; ++op) {
            int phase = op / PHASE_LENGTH;
            int putProbability;
            switch (phase) {
                case 0: putProbability = 20; break;
                case 1: putProbability = 35; break;
                case 2: putProbability = 10; break;
                case 3: putProbability = 30; break;
                case 4: putProbability = 25; break;
                case 5: putProbability = 15; break;
                default: putProbability = 20;
            }
            
            bool isPut = (gen() % 100 < putProbability);
            int key;
            
            if (op < PHASE_LENGTH) {
                key = gen() % 10;
            } else if (op < PHASE_LENGTH * 2) {
                key = gen() % 500;
            } else if (op < PHASE_LENGTH * 3) {
                key = (op - PHASE_LENGTH * 2) % 200;
            } else if (op < PHASE_LENGTH * 4) {
                int locality = (op / 1000) % 8;
                key = locality * 25 + (gen() % 25);
            } else if (op < PHASE_LENGTH * 5) {
                int r = gen() % 100;
                if (r < 40) {
                    key = gen() % 10;
                } else if (r < 70) {
                    key = 10 + (gen() % 90);
                } else {
                    key = 100 + (gen() % 400);
                }
            } else {
                key = gen() % 15;
            }
            
            if (isPut) {
                std::string value = "value" + std::to_string(key) + "_p" + std::to_string(phase);
                hash_lfu.put(key, value);
            } else {
                std::string result;
                get_operations[5]++;
                if (hash_lfu.get(key, result)) {
                    hits[5]++;
                }
            }
        }
        
        execution_times[5] = timer.elapsed();
    }

    printResults("工作负载剧烈变化测试", CAPACITY, get_operations, hits, execution_times);
}

void testConcurrentPerformance() {
    std::cout << "\n=== 测试场景4：并发性能测试 ===" << std::endl;
    
    const int CAPACITY = 200;
    const int OPERATIONS_PER_THREAD = 5000;
    const int THREAD_COUNT = 4;
    const int KEY_RANGE = 1000;
    
    // 只测试支持并发的缓存
    QCache::LruCache<int, std::string> lru(CAPACITY);
    QCache::LfuCache<int, std::string> lfu(CAPACITY);
    QCache::HashLruCaches<int, std::string> hash_lru(CAPACITY, THREAD_COUNT);
    QCache::HashLfuCache<int, std::string> hash_lfu(CAPACITY, THREAD_COUNT);
    
    std::vector<int> hits(4, 0);
    std::vector<int> get_operations(4, 0);
    std::vector<double> execution_times(4, 0.0);
    std::vector<std::string> names = {"LRU", "LFU", "Hash-LRU", "Hash-LFU"};
    
    // 测试LRU
    {
        Timer timer;
        std::atomic<int> total_hits{0};
        std::atomic<int> total_gets{0};
        
        std::vector<std::future<void>> futures;
        
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                
                int local_hits = 0;
                int local_gets = 0;
                
                for (int op = 0; op < OPERATIONS_PER_THREAD; ++op) {
                    bool isPut = (gen() % 100 < 30);
                    int key = gen() % KEY_RANGE;
                    
                    if (isPut) {
                        std::string value = "thread" + std::to_string(t) + "_value" + std::to_string(key);
                        lru.put(key, value);
                    } else {
                        std::string result_value;
                        if (lru.get(key, result_value)) {
                            local_hits++;
                        }
                        local_gets++;
                    }
                }
                
                total_hits += local_hits;
                total_gets += local_gets;
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        hits[0] = total_hits.load();
        get_operations[0] = total_gets.load();
        execution_times[0] = timer.elapsed();
    }
    
    // 测试LFU
    {
        Timer timer;
        std::atomic<int> total_hits{0};
        std::atomic<int> total_gets{0};
        
        std::vector<std::future<void>> futures;
        
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                
                int local_hits = 0;
                int local_gets = 0;
                
                for (int op = 0; op < OPERATIONS_PER_THREAD; ++op) {
                    bool isPut = (gen() % 100 < 30);
                    int key = gen() % KEY_RANGE;
                    
                    if (isPut) {
                        std::string value = "thread" + std::to_string(t) + "_value" + std::to_string(key);
                        lfu.put(key, value);
                    } else {
                        std::string result_value;
                        if (lfu.get(key, result_value)) {
                            local_hits++;
                        }
                        local_gets++;
                    }
                }
                
                total_hits += local_hits;
                total_gets += local_gets;
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        hits[1] = total_hits.load();
        get_operations[1] = total_gets.load();
        execution_times[1] = timer.elapsed();
    }
    
    // 测试Hash-LRU
    {
        Timer timer;
        std::atomic<int> total_hits{0};
        std::atomic<int> total_gets{0};
        
        std::vector<std::future<void>> futures;
        
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                
                int local_hits = 0;
                int local_gets = 0;
                
                for (int op = 0; op < OPERATIONS_PER_THREAD; ++op) {
                    bool isPut = (gen() % 100 < 30);
                    int key = gen() % KEY_RANGE;
                    
                    if (isPut) {
                        std::string value = "thread" + std::to_string(t) + "_value" + std::to_string(key);
                        hash_lru.put(key, value);
                    } else {
                        std::string result_value;
                        if (hash_lru.get(key, result_value)) {
                            local_hits++;
                        }
                        local_gets++;
                    }
                }
                
                total_hits += local_hits;
                total_gets += local_gets;
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        hits[2] = total_hits.load();
        get_operations[2] = total_gets.load();
        execution_times[2] = timer.elapsed();
    }
    
    // 测试Hash-LFU
    {
        Timer timer;
        std::atomic<int> total_hits{0};
        std::atomic<int> total_gets{0};
        
        std::vector<std::future<void>> futures;
        
        for (int t = 0; t < THREAD_COUNT; ++t) {
            futures.push_back(std::async(std::launch::async, [&, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                
                int local_hits = 0;
                int local_gets = 0;
                
                for (int op = 0; op < OPERATIONS_PER_THREAD; ++op) {
                    bool isPut = (gen() % 100 < 30);
                    int key = gen() % KEY_RANGE;
                    
                    if (isPut) {
                        std::string value = "thread" + std::to_string(t) + "_value" + std::to_string(key);
                        hash_lfu.put(key, value);
                    } else {
                        std::string result_value;
                        if (hash_lfu.get(key, result_value)) {
                            local_hits++;
                        }
                        local_gets++;
                    }
                }
                
                total_hits += local_hits;
                total_gets += local_gets;
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        hits[3] = total_hits.load();
        get_operations[3] = total_gets.load();
        execution_times[3] = timer.elapsed();
    }
    
    std::cout << std::left << std::setw(12) << "算法" 
              << std::setw(10) << "命中率%" 
              << std::setw(15) << "命中次数/总次数"
              << std::setw(12) << "执行时间(ms)"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    for (int i = 0; i < 4; ++i) {
        double hitRate = get_operations[i] > 0 ? 100.0 * hits[i] / get_operations[i] : 0.0;
        std::cout << std::left << std::setw(12) << names[i]
                  << std::setw(10) << std::fixed << std::setprecision(2) << hitRate
                  << std::setw(15) << (std::to_string(hits[i]) + "/" + std::to_string(get_operations[i]))
                  << std::setw(12) << std::fixed << std::setprecision(1) << execution_times[i]
                  << std::endl;
    }
    
    std::cout << "\n并发测试说明:" << std::endl;
    std::cout << "• 分片缓存通过减少锁竞争来提高并发性能" << std::endl;
    std::cout << "• 在高并发场景下，分片缓存通常有更好的吞吐量" << std::endl;
}

int main() {
    std::cout << "QCache 缓存算法综合性能测试" << std::endl;
    std::cout << "测试包括：LRU、LFU、LRU-K、ARC、分片LRU、分片LFU" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    try {
        testHotDataAccess();
        testLoopPattern();
        testWorkloadShift();
        testConcurrentPerformance();
        
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "=== 综合测试总结 ===" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << "\n算法特点分析:" << std::endl;
        std::cout << "• LRU: 简单高效，适合时间局部性强的场景" << std::endl;
        std::cout << "• LFU: 适合访问频率差异明显的场景，但需要时间建立统计" << std::endl;
        std::cout << "• LRU-K: 在循环扫描等场景下表现优异，能有效过滤一次性访问" << std::endl;
        std::cout << "• ARC: 自适应算法，在多种场景下都有不错的表现" << std::endl;
        std::cout << "• Hash-LRU/LFU: 通过分片提高并发性能，适合高并发场景" << std::endl;
        
        std::cout << "\n选择建议:" << std::endl;
        std::cout << "• 通用场景: 推荐使用ARC，平衡性能好" << std::endl;
        std::cout << "• 高并发场景: 推荐使用分片缓存" << std::endl;
        std::cout << "• 循环访问场景: 推荐使用LRU-K" << std::endl;
        std::cout << "• 简单场景: LRU足够且高效" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 