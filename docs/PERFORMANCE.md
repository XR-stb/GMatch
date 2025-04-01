# GMatch 性能优化指南

本文档提供了GMatch服务的性能优化指南，包括服务器配置、代码优化和系统调优方面的建议。

## 基准测试

在进行任何优化之前，应该先进行基准测试，了解系统当前的性能状况。GMatch提供了自动化测试脚本，可以用来测试系统的性能：

```bash
./scripts/test_match.sh --clients 1000 --duration 300
```

基准测试应该记录以下指标：
- 每秒处理的请求数（RPS）
- 请求延迟（平均、p95、p99）
- CPU使用率
- 内存使用率
- 网络吞吐量

## 服务器优化

### 配置优化

1. **线程池大小**

   线程池大小应根据CPU核心数进行调整。一般建议设置为CPU核心数的1.5-2倍：

   ```ini
   [server]
   thread_pool_size = 16  # 对于8核CPU
   ```

2. **套接字缓冲区**

   增加套接字缓冲区大小可以提高网络性能：

   ```ini
   [server]
   socket_recv_buffer_size = 262144  # 256KB
   socket_send_buffer_size = 262144  # 256KB
   ```

3. **最大连接数**

   根据预期的并发用户数调整最大连接数：

   ```ini
   [server]
   max_connections = 10000
   ```

4. **匹配间隔**

   调整匹配算法执行的时间间隔，平衡匹配质量和CPU使用率：

   ```ini
   [match]
   match_interval_ms = 500  # 每0.5秒执行一次匹配
   ```

### 系统配置

1. **文件描述符限制**

   增加系统的文件描述符限制，支持更多并发连接：

   ```bash
   # /etc/security/limits.conf
   * soft nofile 65535
   * hard nofile 65535
   ```

2. **TCP调优**

   优化TCP参数，提高网络性能：

   ```bash
   # /etc/sysctl.conf
   net.ipv4.tcp_tw_reuse = 1
   net.ipv4.tcp_fin_timeout = 30
   net.core.somaxconn = 65535
   net.ipv4.tcp_max_syn_backlog = 65535
   ```

3. **内存配置**

   调整内存相关参数：

   ```bash
   # /etc/sysctl.conf
   vm.swappiness = 10
   vm.dirty_ratio = 40
   vm.dirty_background_ratio = 10
   ```

## 代码优化

### 内存优化

1. **对象池**

   为频繁创建和销毁的对象（如玩家连接）使用对象池：

   ```cpp
   // 优化前
   Connection* conn = new Connection();
   // 使用conn
   delete conn;

   // 优化后
   Connection* conn = connectionPool.acquire();
   // 使用conn
   connectionPool.release(conn);
   ```

2. **避免内存碎片**

   使用自定义内存分配器，减少内存碎片：

   ```cpp
   template <typename T>
   class PoolAllocator {
       // 实现内存池分配器
   };

   std::vector<Player, PoolAllocator<Player>> players;
   ```

3. **减少不必要的复制**

   使用移动语义和引用传递减少不必要的复制操作：

   ```cpp
   // 优化前
   void processRequest(std::string request);

   // 优化后
   void processRequest(const std::string& request);
   ```

### 并发优化

1. **无锁数据结构**

   在高并发场景中使用无锁数据结构：

   ```cpp
   // 优化前
   std::vector<Player> players;
   std::mutex playersMutex;

   // 优化后
   tbb::concurrent_vector<Player> players;
   ```

2. **细粒度锁**

   使用细粒度锁代替粗粒度锁：

   ```cpp
   // 优化前
   std::mutex globalMutex;

   // 优化后
   std::array<std::mutex, 64> shardedMutexes;
   std::mutex& getMutexForPlayer(const std::string& playerId) {
       // 根据playerId哈希选择一个mutex
       return shardedMutexes[std::hash<std::string>{}(playerId) % shardedMutexes.size()];
   }
   ```

3. **读写锁**

   对于读多写少的场景，使用读写锁：

   ```cpp
   // 优化前
   std::mutex roomsMutex;

   // 优化后
   std::shared_mutex roomsMutex;

   // 读操作
   {
       std::shared_lock<std::shared_mutex> lock(roomsMutex);
       // 读取rooms
   }

   // 写操作
   {
       std::unique_lock<std::shared_mutex> lock(roomsMutex);
       // 修改rooms
   }
   ```

### 算法优化

1. **匹配算法优化**

   优化匹配算法，减少时间复杂度：

   ```cpp
   // 优化前 - O(n²)时间复杂度
   for (auto& player1 : players) {
       for (auto& player2 : players) {
           if (isMatch(player1, player2)) {
               // 匹配成功
           }
       }
   }

   // 优化后 - 使用评分区间索引，降低到O(n log n)
   std::map<int, std::vector<Player*>> ratingBuckets;
   for (auto& player : players) {
       int bucket = player.rating / 100;  // 100分一个桶
       ratingBuckets[bucket].push_back(&player);
   }

   for (auto& [bucket, bucketPlayers] : ratingBuckets) {
       // 只检查相邻桶的玩家
       for (int i = bucket - 3; i <= bucket + 3; i++) {
           if (ratingBuckets.count(i) == 0) continue;
           for (auto& otherPlayer : ratingBuckets[i]) {
               // 匹配逻辑
           }
       }
   }
   ```

2. **使用更高效的数据结构**

   根据访问模式选择合适的数据结构：

   ```cpp
   // 优化前 - 使用map查询玩家
   std::map<std::string, Player> playersById;

   // 优化后 - 使用unordered_map提高查询性能
   std::unordered_map<std::string, Player> playersById;
   ```

3. **批处理**

   使用批处理减少系统调用和锁争用：

   ```cpp
   // 优化前 - 逐条处理日志
   for (const auto& logMsg : logMessages) {
       logger.log(logMsg);
   }

   // 优化后 - 批量处理日志
   logger.logBatch(logMessages);
   ```

## 网络优化

1. **消息合并**

   将多个小消息合并成一个大消息，减少网络开销：

   ```cpp
   // 优化前
   for (const auto& event : events) {
       sendToClient(clientId, event);
   }

   // 优化后
   json batchEvents;
   batchEvents["events"] = events;
   sendToClient(clientId, batchEvents);
   ```

2. **消息压缩**

   对大消息进行压缩，减少网络传输量：

   ```cpp
   // 启用消息压缩
   [server]
   enable_compression = true
   compression_threshold = 1024  # 大于1KB的消息进行压缩
   ```

3. **连接复用**

   使用连接池复用连接，减少连接建立和断开的开销：

   ```cpp
   // 客户端配置
   [client]
   connection_pool_size = 10
   connection_timeout_ms = 30000
   ```

## 监控与分析

1. **性能指标收集**

   收集关键性能指标，如请求延迟、队列长度、匹配时间等：

   ```ini
   [monitoring]
   enable_metrics = true
   metrics_interval_ms = 5000  # 每5秒收集一次指标
   ```

2. **热点分析**

   使用profiling工具识别代码热点：

   ```bash
   # 使用perf进行分析
   perf record -g -p <server_pid>
   perf report
   ```

3. **内存分析**

   使用内存分析工具检测内存泄漏和内存使用模式：

   ```bash
   # 使用Valgrind进行内存分析
   valgrind --tool=massif ./match_server
   ```

## 扩展性优化

1. **水平扩展**

   设计系统支持水平扩展，通过增加服务器实例扩展系统容量：

   ```ini
   [cluster]
   enable_cluster = true
   node_id = node1
   cluster_nodes = node1:8080,node2:8080,node3:8080
   ```

2. **分区**

   使用分区（Sharding）技术，将玩家按某种规则分配到不同的服务器：

   ```cpp
   // 根据玩家ID哈希决定服务器
   size_t getServerIndex(const std::string& playerId) {
       return std::hash<std::string>{}(playerId) % serverCount;
   }
   ```

3. **异步处理**

   对于非实时操作，使用异步处理减轻主线程负担：

   ```cpp
   // 异步处理日志
   std::async(std::launch::async, [this, logMsg]() {
       this->writeLogToFile(logMsg);
   });
   ```

## 总结

性能优化是一个持续的过程，应该遵循以下步骤：

1. 测量 - 通过基准测试和性能监控确定系统瓶颈
2. 分析 - 分析性能数据，找出性能问题的根本原因
3. 优化 - 针对性地实施优化措施
4. 验证 - 再次测量，确认优化效果
5. 重复 - 不断循环上述过程，直到达到期望的性能目标

优化时应注意：

1. 过早优化是万恶之源，应该先确定瓶颈再优化
2. 优化应该基于数据，而不是直觉
3. 始终保持代码的可维护性和可读性
4. 记录优化前后的性能数据，便于未来参考 