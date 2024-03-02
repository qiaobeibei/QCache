//
// Created by beauqiao on 25-8-6.
//

#ifndef LRUKCACHE_H
#define LRUKCACHE_H
#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <string>
#include "LruCache.h"

namespace QCache {
  template <typename Key, typename Value>
  class LruCache;

  /**
   * @brief LRU-K缓存实现
   * 通过记录访问历史，只有被访问K次的数据才会进入缓存
   */
  template <typename Key, typename Value>
  class LruKCache : public LruCache<Key, Value> {
  public:
    /**
     * @brief 构造函数
     *
     * @param capacity 缓存队列容量（满K次访问）
     * @param history_capacity 历史队列容量（未满K次访问）
     * @param k 进入缓存队列的访问次数阈值
     */
    LruKCache(int capacity, int history_capacity, int k) :
      LruCache<Key, Value>(capacity), // 缓存队列
      history_cache_(std::make_unique<LruCache<Key, size_t>>(history_capacity)), // 未满足K次访问的key的访问次数
      k_(k) {
    }

    /**
     * @brief 获取缓存值
     *
     * @param key
     * @return Value 缓存值，如果键不存在则返回默认值
     */
    Value get(Key key) override {
      // 首先尝试从缓存队列获取数据
      Value value{};
      bool in_main_cache = LruCache<Key, Value>::get(key, value);

      // 若数据在缓存队列中，直接返回
      if (in_main_cache) {
        return value;
      }

      // 否则获取历史队列key对应的访问次数并更新
      size_t history_count = history_cache_->get(key);
      ++history_count;
      history_cache_->put(key, history_count);

      // 如果数据不在缓存队列中，但访问次数达到k
      if (history_count >= k_) {
        auto it = history_value_map_.find(key);
        if (it != history_value_map_.end()) {
          // 从历史队列移除，添加至缓存队列
          Value stored_value = it->second;
          history_cache_->remove(key);
          history_value_map_.erase(it);
          LruCache<Key, Value>::put(key, stored_value);

          return stored_value;
        }
      }

      // 数据不在缓存队列也不在历史队列时返回默认值
      return value;
    }

    bool get(Key key, Value &value) override {
      // 首先尝试从缓存队列获取数据
      bool in_main_cache = LruCache<Key, Value>::get(key, value);

      // 若数据在缓存队列中，直接返回
      if (in_main_cache) {
        return true;
      }

      // 否则获取历史队列key对应的访问次数并更新
      size_t history_count = history_cache_->get(key);
      ++history_count;
      history_cache_->put(key, history_count);

      // 如果数据不在缓存队列中，但访问次数达到k
      if (history_count >= k_) {
        auto it = history_value_map_.find(key);
        if (it != history_value_map_.end()) {
          // 从历史队列移除，添加至缓存队列
          value = it->second;
          history_cache_->remove(key);
          history_value_map_.erase(it);
          LruCache<Key, Value>::put(key, value);
          return true;
        }
      }
      return false;
    }

    /**
     * 添加或更新缓存
     * @param key
     * @param value
     * @return
     */
    void put(Key key, Value value) override {
      Value existing_value{};
      bool in_main_cache = LruCache<Key, Value>::get(key, existing_value);

      if (in_main_cache) {
        LruCache<Key, Value>::put(key, value);
        return;
      }

      size_t history_count = history_cache_->get(key);
      ++history_count;
      // 更新历史队列数据，同时也是添加新数据（如果不存在）
      history_cache_->put(key, history_count);
      history_value_map_[key] = value;

      if (history_count >= k_) {
        history_cache_->remove(key);
        history_value_map_.erase(key);
        LruCache<Key, Value>::put(key, value);
      }
    }

  private:
    int k_;
    std::unique_ptr<LruCache<Key, size_t>> history_cache_; // 历史队列，存放未命中的key的访问次数
    std::unordered_map<Key, Value> history_value_map_; // 历史队列，存放未满k次访问的key的数据
  };

} // namespace QCache

#endif //LRUKCACHE_H
