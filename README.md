# QCache - 高性能C++缓存库

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.28+-green.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

QCache是一个基于C++17实现的高性能、线程安全的缓存库，提供多种缓存算法实现，适用于各种应用场景。

## 🚀 特性

- **多种缓存算法**：支持LRU、LFU、LRU-K、ARC等经典缓存算法
- **分片缓存**：提供Hash-LRU和Hash-LFU分片实现，提升并发性能
- **线程安全**：所有缓存实现都是线程安全的
- **高性能**：经过优化的数据结构和算法实现
- **易于使用**：简洁的API设计，支持泛型
- **全面测试**：包含功能测试和性能基准测试

## 📋 支持的缓存算法

| 算法           | 描述          | 适用场景         |
|--------------|-------------|--------------|
| **LRU**      | 最近最少使用算法    | 时间局部性强的场景    |
| **LFU**      | 最少使用频率算法    | 访问频率差异明显的场景  |
| **LRU-K**    | K距离最近最少使用算法 | 循环访问、过滤一次性访问 |
| **ARC**      | 自适应替换缓存算法   | 通用场景，自适应性能   |
| **Hash-LRU** | 分片LRU缓存     | 高并发LRU场景     |
| **Hash-LFU** | 分片LFU缓存     | 高并发LFU场景     |

## 🛠️ 构建要求

- **编译器**：支持C++17的编译器（GCC 7+, Clang 5+, MSVC 2017+）
- **构建系统**：CMake 3.28+
- **操作系统**：Linux, macOS, Windows

## 📦 快速开始

### 编译项目

```bash
git clone <repository-url>
cd QCache
mkdir build && cd build
cmake ..
make
```

### 基本使用示例

```cpp
#include "LruCache.h"
#include "LfuCache.h"
#include "ArcCache.h"

// LRU缓存示例
QCache::LruCache<int, std::string> lru_cache(100);  // 容量100
lru_cache.put(1, "value1");
std::string value;
if (lru_cache.get(1, value)) {
    std::cout << "Found: " << value << std::endl;
}

// LFU缓存示例
QCache::LfuCache<int, std::string> lfu_cache(100);
lfu_cache.put(1, "value1");
auto result = lfu_cache.get(1);  // 返回值方式

// ARC缓存示例
QCache::ArcCache<int, std::string> arc_cache(100);
arc_cache.put(1, "value1");

// 分片缓存示例（高并发）
QCache::HashLruCache<int, std::string> hash_lru(100, 4);  // 容量100，4个分片
QCache::HashLfuCache<int, std::string> hash_lfu(100, 4);  // 容量100，4个分片
```

## 🧪 运行测试

项目包含两个主要测试程序：

### 基本功能测试

```bash
./BasicTest
```

测试各算法的基本功能、边界条件和正确性。

### 综合性能测试

```bash
./ComprehensiveTest
```

对比各算法在不同场景下的性能表现。

## 📊 性能基准测试

基于最新的性能测试结果（缓存容量20-100，操作数10万-100万次）：

### 热点数据访问场景

| 算法       | 命中率   | 执行时间  | 适用性   |
|----------|-------|-------|-------|
| LRU      | 49.5% | 235ms | ⭐⭐⭐⭐  |
| LFU      | 66.9% | 344ms | ⭐⭐⭐⭐⭐ |
| ARC      | 66.0% | 869ms | ⭐⭐⭐⭐  |
| LRU-K    | 54.7% | 565ms | ⭐⭐⭐   |
| Hash-LRU | 48.8% | 238ms | ⭐⭐⭐⭐  |
| Hash-LFU | 50.0% | 优化中   | ⭐⭐⭐   |

### 循环扫描场景

| 算法       | 命中率   | 执行时间  | 适用性  |
|----------|-------|-------|------|
| LRU      | 4.4%  | 111ms | ⭐⭐   |
| LFU      | 10.9% | 148ms | ⭐⭐⭐  |
| ARC      | 10.8% | 286ms | ⭐⭐⭐  |
| LRU-K    | 7.2%  | 481ms | ⭐⭐⭐⭐ |
| Hash-LRU | 4.4%  | 113ms | ⭐⭐   |
| Hash-LFU | 10.8% | 155ms | ⭐⭐⭐  |

### 并发性能测试

| 算法       | 命中率   | 执行时间 | 并发优势  |
|----------|-------|------|-------|
| LRU      | 19.7% | 27ms | 基准    |
| LFU      | 19.6% | 32ms | 基准    |
| Hash-LRU | 19.7% | 12ms | ⭐⭐⭐⭐⭐ |
| Hash-LFU | 19.0% | 17ms | ⭐⭐⭐⭐  |

## 🎯 算法选择建议

- **通用场景**：推荐使用**ARC**，自适应性能好
- **高并发场景**：推荐使用**Hash-LRU**或**Hash-LFU**
- **循环访问场景**：推荐使用**LRU-K**
- **简单高效场景**：**LRU**足够且性能优秀
- **频率敏感场景**：使用**LFU**

## 🔧 API 参考

### 基本接口

所有缓存类都实现了统一的接口：

```cpp
template<typename Key, typename Value>
class CachePolicy {
public:
    virtual void put(Key key, Value value) = 0;
    virtual bool get(Key key, Value& value) = 0;
    virtual Value get(Key key) = 0;  
    virtual void purge() = 0;      
    virtual size_t capacity() const = 0;
};
```

### 构造函数参数

```cpp
// 基本缓存
LruCache(int capacity);
LfuCache(int capacity, int max_average_freq = 1000000);
LruKCache(int capacity, int k = 2);
ArcCache(int capacity);

// 分片缓存
HashLruCache(size_t capacity, int slice_num);
HashLfuCache(size_t capacity, int slice_num, int max_average_freq = 1000);
```

## 🐛 已知问题

- Hash-LFU在某些热点数据场景下可能存在性能瓶颈（正在优化中）
- 大容量缓存的内存使用需要进一步优化

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 📞 联系方式

如有问题或建议，可提交Issue。
---
