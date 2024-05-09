//
// Created by beauqiao on 25-8-6.
//

#ifndef ARCCACHENODE_H
#define ARCCACHENODE_H
#include <memory>

namespace QCache {
  /**
   * @brief ArcNode
   *
   * 存储键值对和访问信息的双向链表节点，用于ARC缓存实现
   */
  template <typename Key, typename Value>
  class ArcNode {
  private:
    Key key_;
    Value value_;
    size_t access_count_; // 访问次数
    std::weak_ptr<ArcNode> prev_;
    std::shared_ptr<ArcNode> next_;

  public:
    /**
     * @brief 默认构造函数
     */
    ArcNode() :
      access_count_(1), next_(nullptr) {
    }

    /**
     * @brief 构造函数
     *
     * @param key 缓存键
     * @param value 缓存值
     */
    ArcNode(Key key, Value value) :
      key_(key)
      , value_(value)
      , access_count_(1)
      , next_(nullptr) {
    }

    /**
     * @brief 获取缓存键
     *
     * @return Key 缓存键
     */
    Key getKey() const { return key_; }

    /**
      * @brief 获取缓存值
      *
      * @return Value 缓存值
      */
    Value getValue() const { return value_; }

    /**
     * @brief 获取访问次数
     *
     * @return size_t 访问次数
     */
    [[nodiscard]] size_t getAccessCount() const { return access_count_; }

    /**
     * @brief 设置缓存值
     *
     * @param value 新的缓存值
     */
    void setValue(const Value &value) { value_ = value; }

    /**
     * @brief 增加访问次数
     */
    void incrementAccessCount() { ++access_count_; }

    template <typename K, typename V>
    friend class ArcLruPart;
    template <typename K, typename V>
    friend class ArcLfuPart;
  };
}

#endif //ARCCACHENODE_H
