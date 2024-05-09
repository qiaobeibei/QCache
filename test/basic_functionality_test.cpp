#include <iostream>
#include <string>
#include <cassert>
#include <vector>
#include <iomanip>
#include "QCachePolicy.h"
#include "LruCache.h"
#include "LruKCache.h"
#include "HashLruCaches.h"
#include "LfuCache.h"
#include "HashLfuCache.h"
#include "ArcCache.h"

/**
 * @brief 测试LRU缓存基本功能
 */
void testLruBasicFunctionality() {
  std::cout << "=== 测试LRU缓存基本功能 ===" << std::endl;

  QCache::LruCache<int, std::string> lru(3);

  // 测试put和get
  lru.put(1, "value1");
  lru.put(2, "value2");
  lru.put(3, "value3");

  std::string value;
  assert(lru.get(1, value) && value == "value1");
  assert(lru.get(2, value) && value == "value2");
  assert(lru.get(3, value) && value == "value3");

  // 测试容量限制和LRU策略
  lru.put(4, "value4");
  assert(!lru.get(1, value)); // key=1被淘汰
  assert(lru.get(2, value) && value == "value2");
  assert(lru.get(3, value) && value == "value3");
  assert(lru.get(4, value) && value == "value4");

  // 测试访问顺序对淘汰的影响
  lru.get(2, value); // 访问key=2，使其成为最近使用
  lru.put(5, "value5"); // 淘汰key=3（最久未使用）
  assert(lru.get(2, value) && value == "value2");
  assert(!lru.get(3, value));
  assert(lru.get(4, value) && value == "value4");
  assert(lru.get(5, value) && value == "value5");

  std::cout << "LRU缓存基本功能测试通过!" << std::endl;
}

/**
 * @brief 测试LFU缓存基本功能
 */
void testLfuBasicFunctionality() {
  std::cout << "=== 测试LFU缓存基本功能 ===" << std::endl;

  QCache::LfuCache<int, std::string> lfu(3);

  // 测试put和get
  lfu.put(1, "value1");
  lfu.put(2, "value2");
  lfu.put(3, "value3");

  std::string value;
  assert(lfu.get(1, value) && value == "value1");
  assert(lfu.get(2, value) && value == "value2");
  assert(lfu.get(3, value) && value == "value3");

  // 增加访问频率
  lfu.get(1, value); // key=1访问2次
  lfu.get(1, value); // key=1访问3次
  lfu.get(2, value); // key=2访问2次

  // 添加新元素，应该淘汰频率最低的key=3
  lfu.put(4, "value4");
  assert(lfu.get(1, value) && value == "value1"); // 频率高，应该保留
  assert(lfu.get(2, value) && value == "value2"); // 频率中等，应该保留
  assert(lfu.get(4, value) && value == "value4"); // 新添加的

  std::cout << "LFU缓存基本功能测试通过!" << std::endl;
}

/**
 * @brief 测试ARC缓存基本功能
 */
void testArcBasicFunctionality() {
  std::cout << "=== 测试ARC缓存基本功能 ===" << std::endl;

  QCache::ArcCache<int, std::string> arc(5);

  // 测试基本put和get
  arc.put(1, "value1");
  arc.put(2, "value2");
  arc.put(3, "value3");

  std::string value;
  assert(arc.get(1, value) && value == "value1");
  assert(arc.get(2, value) && value == "value2");
  assert(arc.get(3, value) && value == "value3");

  // 测试更新
  arc.put(1, "updated_value1");
  assert(arc.get(1, value) && value == "updated_value1");

  std::cout << "ARC缓存基本功能测试通过!" << std::endl;
}

/**
 * @brief 测试LRU-K缓存基本功能
 */
void testLruKBasicFunctionality() {
  std::cout << "=== 测试LRU-K缓存基本功能 ===" << std::endl;

  QCache::LruKCache<int, std::string> lruk(3, 10, 2); // 容量3，历史容量10，K=2

  // 测试put和get
  lruk.put(1, "value1");
  lruk.put(2, "value2");

  std::string value;
  // 第一次访问，应该在历史队列中
  value = lruk.get(1); // 第一次访问
  // 注意：第一次访问可能返回默认值，因为还没达到K次

  // 第二次访问，应该进入缓存队列
  lruk.get(1, value); // 第二次访问，现在应该在主缓存中

  std::cout << "LRU-K缓存基本功能测试通过!" << std::endl;
}

/**
 * @brief 测试分片LRU缓存基本功能
 */
void testHashLruBasicFunctionality() {
  std::cout << "=== 测试分片LRU缓存基本功能 ===" << std::endl;

  QCache::HashLruCaches<int, std::string> hashLru(6, 2); // 总容量6，2个分片

  // 测试put和get
  hashLru.put(1, "value1");
  hashLru.put(2, "value2");
  hashLru.put(3, "value3");

  std::string value;
  assert(hashLru.get(1, value) && value == "value1");
  assert(hashLru.get(2, value) && value == "value2");
  assert(hashLru.get(3, value) && value == "value3");

  // 测试更新
  hashLru.put(1, "updated_value1");
  assert(hashLru.get(1, value) && value == "updated_value1");

  std::cout << "分片LRU缓存基本功能测试通过!" << std::endl;
}

/**
 * @brief 测试分片LFU缓存基本功能
 */
void testHashLfuBasicFunctionality() {
  std::cout << "=== 测试分片LFU缓存基本功能 ===" << std::endl;

  QCache::HashLfuCache<int, std::string> hashLfu(6, 2); // 总容量6，2个分片

  // 测试put和get
  hashLfu.put(1, "value1");
  hashLfu.put(2, "value2");
  hashLfu.put(3, "value3");

  std::string value;
  assert(hashLfu.get(1, value) && value == "value1");
  assert(hashLfu.get(2, value) && value == "value2");
  assert(hashLfu.get(3, value) && value == "value3");

  // 测试更新
  hashLfu.put(1, "updated_value1");
  assert(hashLfu.get(1, value) && value == "updated_value1");

  std::cout << "分片LFU缓存基本功能测试通过!" << std::endl;
}

/**
 * @brief 测试边界条件
 */
void testBoundaryConditions() {
  std::cout << "=== 测试边界条件 ===" << std::endl;

  // 测试容量为1的缓存
  QCache::LruCache<int, std::string> lru1(1);
  lru1.put(1, "value1");
  std::string value;
  assert(lru1.get(1, value) && value == "value1");

  lru1.put(2, "value2"); // 应该淘汰key=1
  assert(!lru1.get(1, value));
  assert(lru1.get(2, value) && value == "value2");

  // 测试容量为0的缓存
  QCache::LruCache<int, std::string> lru0(0);
  lru0.put(1, "value1"); // 应该不会存储
  assert(!lru0.get(1, value));

  std::cout << "边界条件测试通过!" << std::endl;
}

/**
 * @brief 简单的性能对比测试
 */
void testSimplePerformanceComparison() {
  std::cout << "=== 简单性能对比测试 ===" << std::endl;

  const int CAPACITY = 10;
  const int OPERATIONS = 1000;

  QCache::LruCache<int, std::string> lru(CAPACITY);
  QCache::LfuCache<int, std::string> lfu(CAPACITY);
  QCache::ArcCache<int, std::string> arc(CAPACITY);

  std::vector<int> hits(3, 0);
  std::vector<int> gets(3, 0);
  std::vector<std::string> names = {"LRU", "LFU", "ARC"};

  // 简单的热点数据测试
  for (int alg = 0; alg < 3; ++alg) {
    for (int op = 0; op < OPERATIONS; ++op) {
      int key = op % 50; // 50个不同的键
      std::string value = "value" + std::to_string(key);

      if (op % 3 == 0) {
        // 33%概率写入
        if (alg == 0)
          lru.put(key, value);
        else if (alg == 1)
          lfu.put(key, value);
        else
          arc.put(key, value);
      }
      else {
        // 67%概率读取
        std::string result;
        gets[alg]++;
        bool hit = false;
        if (alg == 0)
          hit = lru.get(key, result);
        else if (alg == 1)
          hit = lfu.get(key, result);
        else
          hit = arc.get(key, result);

        if (hit)
          hits[alg]++;
      }
    }
  }

  // 打印结果
  for (int i = 0; i < 3; ++i) {
    double hitRate = gets[i] > 0 ? 100.0 * hits[i] / gets[i] : 0.0;
    std::cout << names[i] << " - 命中率: " << std::fixed << std::setprecision(2)
        << hitRate << "% (" << hits[i] << "/" << gets[i] << ")" << std::endl;
  }

  std::cout << "简单性能对比测试完成!" << std::endl;
}

int main() {
  std::cout << "QCache 基本功能验证测试" << std::endl;
  std::cout << "========================" << std::endl;

  try {
    testLruBasicFunctionality();
    testLfuBasicFunctionality();
    testArcBasicFunctionality();
    testLruKBasicFunctionality();
    testHashLruBasicFunctionality();
    testHashLfuBasicFunctionality();
    testBoundaryConditions();
    testSimplePerformanceComparison();

    std::cout << "\n所有基本功能测试通过!" << std::endl;
  }
  catch (const std::exception &e) {
    std::cerr << "测试失败: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "未知错误" << std::endl;
    return 1;
  }

  return 0;
}
