//
// Created by beauqiao on 25-8-6.
//

#ifndef HASHLFUCACHE_H
#define HASHLFUCACHE_H

#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include "LfuCache.h"

namespace QCache {
  template <typename Key, typename Value>
  class LfuCache;

  /**
   * @brief 分片LFU缓存实现类
   *
   * 将LFU缓存分成多个分片，提高高并发下的性能
   */
  template <typename Key, typename Value>
  class HashLfuCache {
  public:
    /**
     * @brief 构造函数
     *
     * @param capacity 总缓存容量
     * @param slice_num 分片数量，默认为线程数
     * @param max_average_freq 最大平均访问频次
     */
    HashLfuCache(size_t capacity, int slice_num, int max_average_freq = 1000) :
      slice_num_(slice_num > 0 ? slice_num : std::thread::hardware_concurrency()), capacity_(capacity) {
      size_t slice_size = std::ceil(capacity_ / static_cast<double>(slice_num));
      for (int i = 0; i < slice_num_; ++i) {
        lfu_slices_.emplace_back(new LfuCache<Key, Value>(slice_size, max_average_freq));
      }
    }

    /**
     * @brief 在对应的分片中添加或更新缓存
     *
     * @param key 缓存键
     * @param value 缓存值
     */
    void put(Key key, Value value) {
      size_t slice_index = Hash(key) % slice_num_;
      lfu_slices_[slice_index]->put(key, value);
    }

    /**
     * @brief 从对应索引的分片中获取缓存值
     *
     * @param key
     * @param value 存储获取到的值
     * @return true
     * @return false
     */
    bool get(Key key, Value &value) {
      size_t slice_index = Hash(key) % slice_num_;
      return lfu_slices_[slice_index]->get(key, value);
    }

    /**
     * @brief 获取缓存值
     *
     * @param key
     * @return Value 缓存值，如果键不存在则返回默认值
     */
    Value get(Key key) {
      Value value{};
      get(key, value);
      return value;
    }

    /**
     * @brief 清空缓存
     */
    void purge() {
      for (auto &lfu_slice : lfu_slices_) {
        lfu_slice->purge();
      }
    }

    /**
     * @brief 获取总缓存容量
     *
     * @return size_t
     */
    [[nodiscard]] size_t capacity() const {
      return capacity_;
    }

    /**
     * @brief 获取分片数量
     *
     * @return int 分片数量
     */
    [[nodiscard]] int sliceCount() const {
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

  private:
    size_t capacity_; // 缓存总容量
    int slice_num_; // 缓存分片数量
    std::vector<std::unique_ptr<LfuCache<Key, Value>>> lfu_slices_;
  };

}

#endif //HASHLFUCACHE_H
