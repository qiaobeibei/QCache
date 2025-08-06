//
// Created by beauqiao on 25-8-5.
//

#ifndef HASHLRUCACHES_H
#define HASHLRUCACHES_H

#include <cmath>
#include <memory>
#include <thread>
#include <string>
#include "LruCache.h"

namespace QCache {
  /**
   * @brief 分片LRU缓存实现类
   *
   * 将LRU缓存分成多个分片，提高高并发下的性能
   */
  template <typename Key, typename Value>
  class HashLruCaches {
  public:
    /**
     * @brief 构造函数
     *
     * @param capacity 总缓存容量
     * @param slice_num 分片数量，默认为硬件线程数
     */
    HashLruCaches(size_t capacity, int slice_num) :
      capacity_(capacity), slice_num_(slice_num > 0 ? slice_num : std::thread::hardware_concurrency()) {
      size_t slice_size = std::ceil(capacity / static_cast<double>(slice_num_)); // 获取每个分片大小
      for (int i = 0; i < slice_num_; ++i) {
        lru_slices_.emplace_back(new LruCache<Key, Value>(slice_size));
      }
    }

    /**
     * @brief 往对应分片中添加或更新缓存
     *
     * @param key 缓存键
     * @param value 缓存值
     */
    void put(Key key, Value value) {
      // 计算对应分片的索引，并添加或更新缓存
      size_t slice_index = Hash(key) % slice_num_;
      lru_slices_[slice_index]->put(key, value);
    }

    /**
     * @brief 获取缓存值
     *
     * @param key 缓存键
     * @param value 输出参数，存储获取到的值
     * @return true 如果键存在
     * @return false 如果键不存在
     */
    bool get(Key key, Value &value) {
      size_t slice_index = Hash(key) % slice_num_;
      return lru_slices_[slice_index]->get(key, value);
    }

    /**
     * @brief 获取缓存值
     *
     * @param key 缓存键
     * @return Value 缓存值，如果键不存在则返回默认值
     */
    Value get(Key key) {
      Value value{};
      get(key, value);
      return value;
    }

    /**
     * @brief 获取总缓存容量
     *
     * @return size_t 总缓存容量
     */
    [[nodiscard]] size_t Capacity() const {
      return capacity_;
    }

    /**
     * @brief 获取分片数量
     *
     * @return int 分片数量
     */
    [[nodiscard]] int SliceCount() const {
      return slice_num_;
    }

  private:
    /**
     * @brief 计算键的哈希值
     *
     * @param key 缓存键
     * @return size_t 哈希值
     */
    size_t Hash(Key key) {
      std::hash<Key> hash_func;
      return hash_func(key);
    }

    size_t capacity_; // 总容量
    int slice_num_; // 切片数量
    std::vector<std::unique_ptr<LruCache<Key, Value>>> lru_slices_; // 切片LRU缓存
  };
} // namespace QCache


#endif // HASHLRUCACHES_H
