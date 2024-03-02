//
// Created by beauqiao on 25-8-5.
//

#ifndef QCACHEPOLICY_H
#define QCACHEPOLICY_H

/**
 * @brief 缓存的基本操作接口，所有具体缓存策略都应实现此接口
 */
namespace QCache {
  template <typename Key, typename Value>
  class QCachePolicy {
  public:
    virtual ~QCachePolicy() = default;

    /**
     * @brief 添加或更新缓存
     * @param key
     * @param value
     */
    virtual void put(Key key, Value value) = 0;

    /**
     * @brief 获取缓存值,访问到的值以传出参数的形式返回|访问成功返回true
     * @param key
     * @param value
     * @return
     */
    virtual bool get(Key key, Value &value) = 0;

    /**
     * @param key
     * @return
     */
    virtual Value get(Key key) = 0;
  };
};

#endif //QCACHEPOLICY_H
