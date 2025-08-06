//
// Created by beauqiao on 25-8-6.
//

#ifndef ARCLFUPART_H
#define ARCLFUPART_H
#include "ArcCacheNode.h"
#include <unordered_map>
#include <map>
#include <list>
#include <mutex>

namespace QCache {
  /**
   * @brief ARC缓存的LFU部分
   *
   * 实现ARC缓存中的LFU（最不经常使用）部分，包括主缓存和幽灵缓存
   */
  template <typename Key, typename Value>
  class ArcLfuPart {
  public:
    using NodeType = ArcNode<Key, Value>;
    using NodePtr = std::shared_ptr<NodeType>;
    using NodeMap = std::unordered_map<Key, NodePtr>;
    using FreqMap = std::map<size_t, std::list<NodePtr>>;

    /**
     * @brief 构造函数
     *
     * @param capacity 主缓存容量
     * @param transform_threshold 转换阈值
     */
    explicit ArcLfuPart(size_t capacity, size_t transform_threshold) :
      capacity_(capacity)
      , ghost_capacity_(capacity)
      , transform_threshold_(transform_threshold)
      , min_freq_(0) {
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
     * @return true 如果键存在
     * @return false 如果键不存在
     */
    bool get(Key key, Value &value) {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = main_cache_.find(key);
      if (it != main_cache_.end()) {
        updateNodeFrequency(it->second);
        value = it->second->getValue();
        return true;
      }
      return false;
    }

    /**
     * @brief 检查键是否在主缓存中
     *
     * @param key 缓存键
     * @return true 如果键在主缓存中
     * @return false 如果键不在主缓存中
     */
    bool contain(Key key) {
      return main_cache_.find(key) != main_cache_.end();
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
        evictLeastFrequent();
      }
      --capacity_;
      return true;
    }

  private:
    /**
     * @brief 初始化幽灵链表
     */
    void initializeLists() {
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
      updateNodeFrequency(node);
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
        evictLeastFrequent();
      }

      NodePtr new_node = std::make_shared<NodeType>(key, value);
      main_cache_[key] = new_node;

      // 将新节点添加到频率为1的列表中
      if (freq_map_.find(1) == freq_map_.end()) {
        freq_map_[1] = std::list<NodePtr>();
      }
      freq_map_[1].push_back(new_node);
      min_freq_ = 1;

      return true;
    }

    /**
     * @brief 更新节点的访问频率
     *
     * @param node 要更新的节点
     */
    void updateNodeFrequency(NodePtr node) {
      size_t old_freq = node->getAccessCount();
      node->incrementAccessCount();
      size_t new_freq = node->getAccessCount();

      // 从旧频率列表中移除
      auto &old_list = freq_map_[old_freq];
      old_list.remove(node);
      if (old_list.empty()) {
        freq_map_.erase(old_freq);
        if (old_freq == min_freq_) {
          min_freq_ = new_freq;
        }
      }

      // 添加到新频率列表
      if (freq_map_.find(new_freq) == freq_map_.end()) {
        freq_map_[new_freq] = std::list<NodePtr>();
      }
      freq_map_[new_freq].push_back(node);
    }

    /**
     * @brief 驱逐最不经常使用的节点
     */
    void evictLeastFrequent() {
      if (freq_map_.empty())
        return;

      // 获取最小频率的列表
      auto &min_freq_list = freq_map_[min_freq_];
      if (min_freq_list.empty())
        return;

      // 移除最少使用的节点
      NodePtr least_node = min_freq_list.front();
      min_freq_list.pop_front();

      // 如果该频率的列表为空，则删除该频率项
      if (min_freq_list.empty()) {
        freq_map_.erase(min_freq_);
        // 更新最小频率
        if (!freq_map_.empty()) {
          min_freq_ = freq_map_.begin()->first;
        }
      }

      // 将节点移到幽灵缓存
      if (ghost_cache_.size() >= ghost_capacity_) {
        removeOldestGhost();
      }
      addToGhost(least_node);

      // 从主缓存中移除
      main_cache_.erase(least_node->GetKey());
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
      node->next_ = ghost_tail_;
      node->prev_ = ghost_tail_->prev_;
      if (!ghost_tail_->prev_.expired()) {
        ghost_tail_->prev_.lock()->next_ = node;
      }
      ghost_tail_->prev_ = node;
      ghost_cache_[node->GetKey()] = node;
    }

    /**
     * @brief 移除最旧的幽灵缓存节点
     */
    void removeOldestGhost() {
      NodePtr oldest_ghost = ghost_head_->next_;
      if (oldest_ghost != ghost_tail_) {
        removeFromGhost(oldest_ghost);
        ghost_cache_.erase(oldest_ghost->GetKey());
      }
    }

  private:
    size_t capacity_; // 主缓存容量
    size_t ghost_capacity_; // 幽灵缓存容量
    size_t transform_threshold_; // 转换阈值
    size_t min_freq_; // 最小访问频率
    std::mutex mutex_; // 互斥锁

    NodeMap main_cache_; // 主缓存映射
    NodeMap ghost_cache_; // 幽灵缓存映射
    FreqMap freq_map_; // 频率映射

    NodePtr ghost_head_; // 幽灵链表头节点
    NodePtr ghost_tail_; // 幽灵链表尾节点
  };
}

#endif //ARCLFUPART_H
