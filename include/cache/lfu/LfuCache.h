//
// Created by beauqiao on 25-8-6.
//

#ifndef LFUCACHE_H
#define LFUCACHE_H
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "FrequencyList.h"
#include "../QCachePolicy.h"

namespace QCache {
  template <typename Key, typename Value>
  class FrequencyList;

  /**
 * @brief LFU缓存实现类
 *
 * 基于LFU缓存策略实现
 */
  template <typename Key, typename Value>
  class LfuCache : public QCachePolicy<Key, Value> {
  public:
    using Node = typename FrequencyList<Key, Value>::Node;
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    /**
     * @brief 构造函数
     *
     * @param capacity 缓存容量
     * @param max_average_freq 最大平均访问频次
     */
    explicit LfuCache(int capacity, int max_average_freq = 1000000) :
      capacity_(capacity), min_freq_(INT8_MAX), max_average_freq_(max_average_freq),
      cur_average_freq_(0), cur_total_freq_(0) {
    }

    /**
     * @brief 析构函数
     */
    ~LfuCache() override = default;

    /**
     * @brief 添加或更新缓存
     *
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
        it->second->value_ = value;
        getInternal(it->second, value);
        return;
      }

      putInternal(key, value);
    }

    /**
     * @brief 获取缓存值
     *
     * @param key
     * @param value 存储获取到的值
     * @return true
     * @return false
     */
    bool get(Key key, Value &value) override {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = node_map_.find(key);
      if (it != node_map_.end()) {
        getInternal(it->second, value);
        return true;
      }
      return false;
    }

    /**
     * @brief 获取缓存值
     *
     * @param key
     * @return Value 缓存值，如果键不存在则返回默认值
     */
    Value get(Key key) override {
      Value value{};
      get(key, value);
      return value;
    }

    /**
     * @brief 清空缓存
     */
    void purge() {
      node_map_.clear();
      freq_lists_.clear();
    }

    /**
    * @brief 获取当前缓存大小
    *
    * @return size_t
    */
    size_t size() const {
      std::lock_guard<std::mutex> lock(mutex_);
      return node_map_.size();
    }

    /**
     * @brief 获取缓存容量
     *
     * @return int
     */
    int capacity() const {
      return capacity_;
    }

  private:
    /**
     * @brief 添加新缓存
     *
     * @param key 缓存键
     * @param value 缓存值
     */
    void putInternal(Key key, Value value) {
      if (node_map_.size() == capacity_) {
        // 缓存已满，删除最不常访问的结点，更新当前平均访问频次和总访问频次
        evictLeastFrequent();
      }
      // 创建新结点，将新结点添加进入，更新最小访问频次
      NodePtr node = std::make_shared<Node>(key, value);
      node_map_[key] = node;
      addToFrequencyList(node);
      incrementAverageFreq();
      min_freq_ = std::min(min_freq_, 1);
    }

    /**
     * @brief 获取缓存并更新节点频率
     *
     * @param node 缓存节点
     * @param value 输出参数，存储获取到的值
     */
    void getInternal(NodePtr node, Value &value) {
      // 找到之后需要将其从低访问频次的链表中删除，并且添加到+1的访问频次链表中，
      // 访问频次+1, 然后把value值返回
      value = node->value_;
      // 从原有访问频次的链表中删除节点
      removeFromFrequencyList(node);
      ++node->freq_;
      addToFrequencyList(node);
      // 如果当前node的访问频次如果等于minFreq+1，并且其前驱链表为空，则说明
      // freq_lists_[node->freq - 1]链表因node的迁移已经空了，需要更新最小访问频次
      if (node->freq_ - 1 == min_freq_ && freq_lists_[node->freq_ - 1]->isEmpty())
        min_freq_++;

      // 总访问频次和当前平均访问频次都随之增加
      incrementAverageFreq();
    }

    /**
     * @brief 移除最不经常使用的缓存
     */
    void evictLeastFrequent() {
      NodePtr node = freq_lists_[min_freq_]->getFirstNode();
      removeFromFrequencyList(node);
      node_map_.erase(node->key_);
      decrementAverageFreq(node->freq_);
    }

    /**
     * @brief 从频率列表中移除节点
     *
     * @param node 要移除的节点
     */
    void removeFromFrequencyList(NodePtr node) {
      // 检查结点是否为空
      if (!node)
        return;

      auto freq = node->freq_;
      freq_lists_[freq]->removeNode(node);
    }

    /**
     * @brief 添加节点到对应频率的列表
     *
     * @param node 要添加的节点
     */
    void addToFrequencyList(NodePtr node) {
      // 检查结点是否为空
      if (!node)
        return;

      // 添加进入相应的频次链表前需要判断该频次链表是否存在
      auto freq = node->freq_;
      if (freq_lists_.find(node->freq_) == freq_lists_.end()) {
        // 不存在则创建
        freq_lists_[node->freq_] = new FrequencyList<Key, Value>(node->freq_);
      }

      freq_lists_[freq]->addNode(node);
    }

    /**
     * @brief 增加平均访问频率
     */
    void incrementAverageFreq() {
      cur_total_freq_++;
      if (node_map_.empty())
        cur_average_freq_ = 0;
      else
        cur_average_freq_ = cur_total_freq_ / node_map_.size();

      if (cur_average_freq_ > max_average_freq_) {
        handleExcessiveAverageFreq();
      }
    }

    /**
     * @brief 减少平均访问频率
     *
     * @param freq 要减少的频率值
     */
    void decrementAverageFreq(int freq) {
      // 减少平均访问频次和总访问频次
      cur_total_freq_ -= freq;
      if (node_map_.empty())
        cur_average_freq_ = 0;
      else
        cur_average_freq_ = cur_total_freq_ / node_map_.size();
    }

    /**
     * @brief 处理平均访问频率超过上限的情况
     */
    void handleExcessiveAverageFreq() {
      if (node_map_.empty())
        return;

      // 当前平均访问频次已经超过了最大平均访问频次，所有结点的访问频次- (max_average_freq_ / 2)
      for (auto it = node_map_.begin(); it != node_map_.end(); ++it) {
        // 检查结点是否为空
        if (!it->second)
          continue;

        NodePtr node = it->second;

        // 先从当前频率列表中移除
        removeFromFrequencyList(node);

        // 减少频率
        node->freq_ -= max_average_freq_ / 2;
        if (node->freq_ < 1)
          node->freq_ = 1;

        // 添加到新的频率列表
        addToFrequencyList(node);
      }

      // 更新最小频率
      updateMinFreq();
    }

    /**
     * @brief 更新最小访问频率
     */
    void updateMinFreq() {
      min_freq_ = INT8_MAX;
      for (const auto &pair : freq_lists_) {
        if (pair.second && !pair.second->isEmpty()) {
          min_freq_ = std::min(min_freq_, pair.first);
        }
      }
      if (min_freq_ == INT8_MAX)
        min_freq_ = 1;
    }

  private:
    int capacity_;
    int min_freq_; // 最小访问频次(用于找到最小访问频次结点)
    int max_average_freq_; // 最大平均访问频次
    int cur_average_freq_; // 当前平均访问频次
    int cur_total_freq_; // 当前访问所有缓存次数总数
    mutable std::mutex mutex_;
    NodeMap node_map_; // key 到 缓存节点的映射
    std::unordered_map<int, FrequencyList<Key, Value> *> freq_lists_; // 访问频次 到 该频次链表的映射
  };


}


#endif //LFUCACHE_H
