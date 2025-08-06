cmake_minimum_required(VERSION 3.28)
project(QCache)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)
include_directories(include/cache)
include_directories(include/cache/lru)
include_directories(include/cache/lfu)
include_directories(include/cache/arc)

add_executable(ComprehensiveTest test/comprehensive_performance_test.cpp
        include/cache/QCachePolicy.h
        include/cache/lru/HashLruCaches.h
        include/cache/lru/LruCache.h
        include/cache/lru/LruKCache.h
        include/cache/lfu/FrequencyList.h
        include/cache/lfu/LfuCache.h
        include/cache/lfu/HashLfuCache.h
        include/cache/arc/ArcCache.h
        include/cache/arc/ArcCacheNode.h
        include/cache/arc/ArcLfuPart.h
        include/cache/arc/ArcLruPart.h)

add_executable(BasicTest test/basic_functionality_test.cpp
        include/cache/QCachePolicy.h
        include/cache/lru/HashLruCaches.h
        include/cache/lru/LruCache.h
        include/cache/lru/LruKCache.h
        include/cache/lfu/FrequencyList.h
        include/cache/lfu/LfuCache.h
        include/cache/lfu/HashLfuCache.h
        include/cache/arc/ArcCache.h
        include/cache/arc/ArcCacheNode.h
        include/cache/arc/ArcLfuPart.h
        include/cache/arc/ArcLruPart.h)

# Hash-LFU调试测试
add_executable(DebugHashLfu test/debug_hash_lfu.cpp
        include/cache/QCachePolicy.h
        include/cache/lfu/FrequencyList.h
        include/cache/lfu/LfuCache.h
        include/cache/lfu/HashLfuCache.h)
