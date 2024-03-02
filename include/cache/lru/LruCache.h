//
// Created by beauqiao on 25-8-6.
//

#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <mutex>
#include <unordered_map>
#include "LruCache.h"
#include "QCachePolicy.h"

namespace QCache {
  template <typename Key, typename Value>
  class LruCache;

  /**
   * @brief LRU缓存节点
   * 存储键值对和访问信息的双向链表节点
   */
  template <typename Key, typename Value>
  class LruNode {
  private:
    Key key_;
    Value value_;
    size_t access_count_; // 访问次数
    std::weak_ptr<LruNode<Key, Value>> prev_;
    std::shared_ptr<LruNode<Key, Value>> next_;

  public:
    /**
     * @brief 构造函数
     * @param key
     * @param value
     */
    LruNode(Key key, Value value):
      key_(key), value_(value), access_count_(1) {
    }

    Key getKey() const { return key_; }
    Value getValue() const { return value_; }
    void setValue(const Value &value) { value_ = value; }
    [[nodiscard]] size_t getAccessCount() const { return access_count_; }
    void incrementAccessCount() { ++access_count_; }

    friend class LruCache<Key, Value>;
  };


  /**
 * @brief LRU缓存实现
 * 基于LRU实现
 */
  template <typename Key, typename Value>
  class LruCache : public QCachePolicy<Key, Value> {
  public:
    using LruNodeType = LruNode<Key, Value>;
    using NodePtr = std::shared_ptr<LruNodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    explicit LruCache(int capacity) :
      capacity_(capacity) {
      initializeList();
    }

    /**
     * @brief 添加缓存
     * @param key
     * @param value
     */
    void put(Key key, Value value) override {
      if (capacity_ <= 0) {
        return;
      }

      std::lock_guard<std::mutex> lock(mutex_);
      auto it = node_map_.find(key);
      if (it != node_map_.end()) {
        updateExistingNode(it->second, value);
        return;
      }

      addNewNode(key, value);
    }

    /**
     * @brief 获取缓存
     * @param key
     * @param value
     * @return
     */
    bool get(Key key, Value &value) override {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = node_map_.find(key);
      if (it != node_map_.end()) {
        moveToMostRecent(it->second);
        value = it->second->getValue();
        return true;
      }
      return false;
    }

    /**
     * @brief 获取缓存
     * @param key
     * @return
     */
    Value get(Key key) override {
      Value value{};
      get(key, value);
      return value;
    }

    /**
     * @brief 删除指定键的缓存
     * @param key
     */
    void remove(Key key) {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = node_map_.find(key);
      if (it != node_map_.end()) {
        removeNode(it->second);
        node_map_.erase(it);
      }
    }

    size_t size() const {
      std::lock_guard<std::mutex> lock(mutex_);
      return node_map_.size();
    }

  protected:
    /**
     * @brief 初始化
     */
    void initializeList() {
      dummy_head_ = std::make_shared<LruNodeType>(Key(), Value());
      dummy_tail_ = std::make_shared<LruNodeType>(Key(), Value());
      dummy_head_->next_ = dummy_tail_;
      dummy_tail_->prev_ = dummy_head_;
    }

    /**
     * @brief 更新现有节点
     */
    void updateExistingNode(NodePtr node, const Value &value) {
      node->setValue(value);
      moveToMostRecent(node);
    }

    /**
     * @brief 添加新节点
     */
    void addNewNode(const Key &key, const Value &value) {
      if (node_map_.size() >= capacity_) {
        evictLeastRecent();
      }

      NodePtr new_node = std::make_shared<LruNodeType>(key, value);
      insertNode(new_node);
      node_map_[key] = new_node;
    }

    /**
     * @brief 移动节点到最前端
     */
    void moveToMostRecent(NodePtr node) {
      removeNode(node);
      insertNode(node);
    }

    /**
     * @brief 删除节点
     */
    void removeNode(NodePtr node) {
      if (!node->prev_.expired() && node->next_) {
        auto prev = node->prev_.lock();
        prev->next_ = node->next_;
        node->next_->prev_ = prev;
        node->next_ = nullptr;
      }
    }

    /**
     * @brief 插入节点
     */
    void insertNode(NodePtr node) {
      node->next_ = dummy_tail_;
      node->prev_ = dummy_tail_->prev_;
      dummy_tail_->prev_.lock()->next_ = node;
      dummy_tail_->prev_ = node;
    }

    /**
     * @brief 删除最不常用的节点
     */
    void evictLeastRecent() {
      NodePtr least_recent = dummy_head_->next_;
      removeNode(least_recent);
      node_map_.erase(least_recent->getKey());
    }

  protected:
    int capacity_;
    NodeMap node_map_;
    mutable std::mutex mutex_;
    NodePtr dummy_head_;
    NodePtr dummy_tail_;
  };

} // namespace QCache


#endif //LRUCACHE_H
