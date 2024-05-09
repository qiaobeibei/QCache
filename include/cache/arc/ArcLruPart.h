//
// Created by beauqiao on 25-8-6.
//

#ifndef ARCLRUPART_H
#define ARCLRUPART_H
#include "ArcCacheNode.h"
#include <unordered_map>
#include <mutex>

namespace QCache {
  /**
   * @brief ARC缓存的LRU部分
   *
   * 实现ARC缓存中的LRU（最近最少使用）部分，包括主缓存和幽灵缓存
   */
  template <typename Key, typename Value>
  class ArcLruPart {
  public:
    using NodeType = ArcNode<Key, Value>;
    using NodePtr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    /**
     * @brief 构造函数
     *
     * @param capacity 主缓存容量
     * @param transform_threshold 转换阈值，节点访问次数达到此值时会转移到LFU部分
     */
    explicit ArcLruPart(size_t capacity, size_t transform_threshold) :
      capacity_(capacity)
      , ghost_capacity_(capacity)
      , transform_threshold_(transform_threshold) {
      initializeLists();
    }

    /**
     * @brief 添加或更新缓存
     *
     * @param key 缓存键
     * @param value 缓存值
     * @return true 操作成功
     * @return false 操作失败
     */
    bool put(Key key, Value value) {
      if (capacity_ == 0)
        return false;

      std::lock_guard<std::mutex> lock(mutex_);
      auto it = main_cache_.find(key);
      if (it != main_cache_.end()) {
        return updateExistingNode(it->second, value);
      }
      return addNewNode(key, value);
    }

    /**
     * @brief 获取缓存值
     *
     * @param key 缓存键
     * @param value 输出参数，存储获取到的值
     * @param should_transform 输出参数，指示是否应该将节点转移到LFU部分
     * @return true 如果键存在
     * @return false 如果键不存在
     */
    bool get(Key key, Value &value, bool &should_transform) {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = main_cache_.find(key);
      if (it != main_cache_.end()) {
        should_transform = updateNodeAccess(it->second);
        value = it->second->getValue();
        return true;
      }
      return false;
    }

    /**
     * @brief 检查键是否在幽灵缓存中
     *
     * @param key 缓存键
     * @return true 如果键在幽灵缓存中
     * @return false 如果键不在幽灵缓存中
     */
    bool checkGhost(Key key) {
      auto it = ghost_cache_.find(key);
      if (it != ghost_cache_.end()) {
        removeFromGhost(it->second);
        ghost_cache_.erase(it);
        return true;
      }
      return false;
    }

    /**
     * @brief 增加缓存容量
     */
    void increaseCapacity() { ++capacity_; }

    /**
     * @brief 减少缓存容量
     *
     * @return true 操作成功
     * @return false 操作失败
     */
    bool decreaseCapacity() {
      if (capacity_ <= 0)
        return false;
      if (main_cache_.size() == capacity_) {
        evictLeastRecent();
      }
      --capacity_;
      return true;
    }

  private:
    /**
     * @brief 初始化双向链表
     */
    void initializeLists() {
      main_head_ = std::make_shared<NodeType>();
      main_tail_ = std::make_shared<NodeType>();
      main_head_->next_ = main_tail_;
      main_tail_->prev_ = main_head_;

      ghost_head_ = std::make_shared<NodeType>();
      ghost_tail_ = std::make_shared<NodeType>();
      ghost_head_->next_ = ghost_tail_;
      ghost_tail_->prev_ = ghost_head_;
    }

    /**
     * @brief 更新现有节点
     *
     * @param node 要更新的节点
     * @param value 新的值
     * @return true 操作成功
     * @return false 操作失败
     */
    bool updateExistingNode(NodePtr node, const Value &value) {
      node->setValue(value);
      moveToFront(node);
      return true;
    }

    /**
     * @brief 添加新节点
     *
     * @param key 缓存键
     * @param value 缓存值
     * @return true 操作成功
     * @return false 操作失败
     */
    bool addNewNode(const Key &key, const Value &value) {
      if (main_cache_.size() >= capacity_) {
        evictLeastRecent(); // 驱逐最近最少访问
      }

      NodePtr new_node = std::make_shared<NodeType>(key, value);
      main_cache_[key] = new_node;
      addToFront(new_node);
      return true;
    }

    /**
     * @brief 更新节点的访问信息
     *
     * @param node 要更新的节点
     * @return true 如果节点应该转移到LFU部分
     * @return false 如果节点应该留在LRU部分
     */
    bool updateNodeAccess(NodePtr node) {
      moveToFront(node);
      node->incrementAccessCount();
      return node->getAccessCount() >= transform_threshold_;
    }

    /**
     * @brief 将节点移动到链表头部（最近使用位置）
     *
     * @param node 要移动的节点
     */
    void moveToFront(NodePtr node) {
      // 先从当前位置移除
      if (!node->prev_.expired() && node->next_) {
        auto prev = node->prev_.lock();
        prev->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        node->next_ = nullptr; // 清空指针，防止悬垂引用
      }

      // 添加到头部
      addToFront(node);
    }

    /**
     * @brief 将节点添加到链表头部
     *
     * @param node 要添加的节点
     */
    void addToFront(NodePtr node) {
      node->next_ = main_head_->next_;
      node->prev_ = main_head_;
      main_head_->next_->prev_ = node;
      main_head_->next_ = node;
    }

    /**
     * @brief 驱逐最近最少使用的节点
     */
    void evictLeastRecent() {
      NodePtr least_recent = main_tail_->prev_.lock();
      if (!least_recent || least_recent == main_head_)
        return;

      // 从主链表中移除
      removeFromMain(least_recent);

      // 添加到幽灵缓存
      if (ghost_cache_.size() >= ghost_capacity_) {
        removeOldestGhost();
      }
      addToGhost(least_recent);

      // 从主缓存映射中移除
      main_cache_.erase(least_recent->getKey());
    }

    /**
     * @brief 从主链表中移除节点
     *
     * @param node 要移除的节点
     */
    void removeFromMain(NodePtr node) {
      if (!node->prev_.expired() && node->next_) {
        auto prev = node->prev_.lock();
        prev->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        node->next_ = nullptr; // 清空指针，防止悬垂引用
      }
    }

    /**
     * @brief 从幽灵缓存中移除节点
     *
     * @param node 要移除的节点
     */
    void removeFromGhost(NodePtr node) {
      if (!node->prev_.expired() && node->next_) {
        auto prev = node->prev_.lock();
        prev->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        node->next_ = nullptr; // 清空指针，防止悬垂引用
      }
    }

    /**
     * @brief 将节点添加到幽灵缓存
     *
     * @param node 要添加的节点
     */
    void addToGhost(NodePtr node) {
      // 重置节点的访问计数
      node->access_count_ = 1;

      // 添加到幽灵缓存的头部
      node->next_ = ghost_head_->next_;
      node->prev_ = ghost_head_;
      ghost_head_->next_->prev_ = node;
      ghost_head_->next_ = node;

      // 添加到幽灵缓存映射
      ghost_cache_[node->getKey()] = node;
    }

    /**
     * @brief 移除最旧的幽灵缓存节点
     */
    void removeOldestGhost() {
      // 使用lock()方法，并添加null检查
      NodePtr oldest_ghost = ghost_tail_->prev_.lock();
      if (!oldest_ghost || oldest_ghost == ghost_head_)
        return;

      removeFromGhost(oldest_ghost);
      ghost_cache_.erase(oldest_ghost->getKey());
    }

  private:
    size_t capacity_; // 主缓存容量
    size_t ghost_capacity_; // 幽灵缓存容量
    size_t transform_threshold_; // 转换阈值
    std::mutex mutex_; // 互斥锁

    NodeMap main_cache_; // 主缓存映射
    NodeMap ghost_cache_; // 幽灵缓存映射

    // 主链表
    NodePtr main_head_; // 主链表头节点
    NodePtr main_tail_; // 主链表尾节点
    // 幽灵链表
    NodePtr ghost_head_; // 幽灵链表头节点
    NodePtr ghost_tail_; // 幽灵链表尾节点
  };
}

#endif //ARCLRUPART_H
