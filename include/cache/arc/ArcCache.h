//
// Created by beauqiao on 25-8-6.
//

#ifndef ARCCACHE_H
#define ARCCACHE_H
#include "ArcLruPart.h"
#include "ArcLfuPart.h"
#include <memory>
#include "QCachePolicy.h"

namespace QCache {
  /**
   * @brief ARC（自适应替换缓存）实现类
   *
   * 结合LRU和LFU优点的自适应缓存策略，能够根据访问模式动态调整缓存分配
   */
  template <typename Key, typename Value>
  class ArcCache : public QCachePolicy<Key, Value> {
  public:
    /**
     * @brief 构造函数
     *
     * @param capacity 缓存总容量
     * @param transform_threshold 转换阈值，节点访问次数达到此值时会从LRU部分转移到LFU部分
     */
    explicit ArcCache(size_t capacity = 10, size_t transform_threshold = 2) :
      capacity_(capacity)
      , transform_threshold_(transform_threshold)
      , lru_part_(std::make_unique<ArcLruPart<Key, Value>>(capacity, transform_threshold))
      , lfu_part_(std::make_unique<ArcLfuPart<Key, Value>>(capacity, transform_threshold)) {
    }

    ~ArcCache() override = default;

    /**
     * @brief 添加或更新缓存
     *
     * @param key 缓存键
     * @param value 缓存值
     */
    void put(Key key, Value value) override {
      checkGhostCaches(key);

      // 检查 LFU 部分是否存在该键
      bool in_lfu = lfu_part_->contain(key);
      // 更新 LRU 部分缓存
      lru_part_->put(key, value);
      // 如果 LFU 部分存在该键，则更新 LFU 部分
      if (in_lfu) {
        lfu_part_->put(key, value);
      }
    }

    /**
     * @brief 获取缓存值
     *
     * @param key 缓存键
     * @param value 输出参数，存储获取到的值
     * @return true 如果键存在
     * @return false 如果键不存在
     */
    bool get(Key key, Value &value) override {
      checkGhostCaches(key);

      bool should_transform = false;
      if (lru_part_->Get(key, value, should_transform)) {
        if (should_transform) {
          lfu_part_->Put(key, value);
        }
        return true;
      }
      return lfu_part_->Get(key, value);
    }

    /**
     * @brief 获取缓存值
     *
     * @param key 缓存键
     * @return Value 缓存值，如果键不存在则返回默认值
     */
    Value get(Key key) override {
      Value value{};
      get(key, value);
      return value;
    }

    /**
     * @brief 获取缓存容量
     *
     * @return size_t 缓存容量
     */
    size_t capacity() const {
      return capacity_;
    }

  private:
    /**
     * @brief 检查幽灵缓存并调整LRU和LFU部分的容量
     *
     * @param key 缓存键
     * @return true 如果键在幽灵缓存中
     * @return false 如果键不在幽灵缓存中
     */
    bool checkGhostCaches(Key key) {
      bool in_ghost = false;
      if (lru_part_->checkGhost(key)) {
        if (lfu_part_->decreaseCapacity()) {
          lru_part_->increaseCapacity();
        }
        in_ghost = true;
      }
      else if (lfu_part_->checkGhost(key)) {
        if (lru_part_->decreaseCapacity()) {
          lfu_part_->increaseCapacity();
        }
        in_ghost = true;
      }
      return in_ghost;
    }

  private:
    size_t capacity_; // 缓存总容量
    size_t transform_threshold_; // 转换阈值
    std::unique_ptr<ArcLruPart<Key, Value>> lru_part_; // LRU部分
    std::unique_ptr<ArcLfuPart<Key, Value>> lfu_part_; // LFU部分
  };
}
#endif //ARCCACHE_H
