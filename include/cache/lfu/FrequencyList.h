//
// Created by beauqiao on 25-8-6.
//

#ifndef FREQUENCYLIST_H
#define FREQUENCYLIST_H
#include <cmath>
#include <memory>
#include "LfuCache.h"

namespace QCache {
  template <typename Key, typename Value>
  class LfuCache;

  /**
   * @brief 频率列表
   *
   * 存储相同访问频率的节点的双向链表
   */
  template <typename Key, typename Value>
  class FrequencyList {
  private:
    /**
     * @brief 频率节点结构
     */
    struct Node {
      int freq_; // 访问频次
      Key key_;
      Value value_;
      std::weak_ptr<Node> pre_;
      std::shared_ptr<Node> next_;

      Node():
        freq_(1), next_(nullptr) {
      }

      Node(Key key, Value value) :
        freq_(1), key_(key), value_(value), next_(nullptr) {
      }
    };

    using NodePtr = std::shared_ptr<Node>;
    int freq_; // 当前列表的频率值
    NodePtr head_; // 虚拟头节点
    NodePtr tail_; // 虚拟尾节点

  public:
    /**
     * @brief 构造函数
     *
     * @param frequency 频率值
     */
    explicit FrequencyList(int frequency) :
      freq_(frequency) {
      head_ = std::make_shared<Node>();
      tail_ = std::make_shared<Node>();
      head_->next_ = tail_;
      tail_->pre_ = head_;
    }

    /**
     * @brief 检查列表是否为空
     *
     * @return true
     * @return false
     */
    [[nodiscard]] bool isEmpty() const {
      return head_->next_ == tail_;
    }

    /**
     * @brief 添加节点到列表尾部
     *
     * @param node
     */
    void addNode(NodePtr node) {
      if (!node || !head_ || !tail_) {
        return;
      }
      node->pre_ = tail_->pre_;
      node->next_ = tail_;
      tail_->pre_.lock()->next_ = node;
      tail_->pre_ = node;
    }

    /**
     * @brief 从列表中移除节点
     *
     * @param node
     */
    void removeNode(NodePtr node) {
      if (!node || !head_ || !tail_) {
        return;
      }
      if (node->pre_.expired() || !node->next_) {
        return;
      }
      auto pre = node->pre_.lock();
      pre->next_ = node->next_;
      node->next_->pre_ = pre;
      node->next_ = nullptr; // 确保显式置空next指针，彻底断开节点与链表的连接
    }

    /**
     * @brief 获取列表中的第一个节点
     *
     * @return NodePtr 第一个节点的指针
     */
    NodePtr getFirstNode() const { return head_->next_; }

    friend class LfuCache<Key, Value>;
  };


}

#endif //FREQUENCYLIST_H
