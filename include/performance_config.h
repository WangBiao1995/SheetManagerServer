#ifndef PERFORMANCE_CONFIG_H
#define PERFORMANCE_CONFIG_H

// 确保 size_t 类型被正确定义
#include <cstddef>
#include <chrono>
#include <string>
#include <thread>
#include <atomic>

// 显式包含 std 命名空间中的 size_t
using std::size_t;

namespace PerformanceConfig {
    
    // 连接池配置
    constexpr size_t DEFAULT_MAX_CONNECTIONS = 10000;        // 最大并发连接数
    constexpr size_t DEFAULT_THREAD_POOL_SIZE = 8;           // 工作线程池大小
    constexpr size_t DEFAULT_TASK_QUEUE_SIZE = 50000;        // 任务队列大小
    
    // 缓冲区配置
    constexpr size_t DEFAULT_READ_BUFFER_SIZE = 64 * 1024;   // 64KB读取缓冲区
    constexpr size_t DEFAULT_WRITE_BUFFER_SIZE = 128 * 1024; // 128KB写入缓冲区
    constexpr size_t MAX_UPLOAD_SIZE = 100 * 1024 * 1024;   // 100MB最大上传大小
    
    // 超时配置
    constexpr int CONNECTION_TIMEOUT_MS = 30000;             // 30秒连接超时
    constexpr int REQUEST_TIMEOUT_MS = 60000;                // 60秒请求超时
    constexpr int KEEP_ALIVE_TIMEOUT_MS = 300000;           // 5分钟保活超时
    
    // 文件I/O配置
    constexpr size_t FILE_CHUNK_SIZE = 1024 * 1024;         // 1MB文件块大小
    constexpr size_t MAX_CONCURRENT_UPLOADS = 100;           // 最大并发上传数
    constexpr size_t MAX_CONCURRENT_DOWNLOADS = 200;         // 最大并发下载数
    
    // 性能监控配置
    constexpr int STATS_UPDATE_INTERVAL_MS = 1000;           // 1秒统计更新间隔
    constexpr size_t MAX_STATS_HISTORY = 3600;               // 1小时统计历史
    
    // 内存池配置
    constexpr size_t MEMORY_POOL_CHUNK_SIZE = 4096;          // 4KB内存块大小
    constexpr size_t MEMORY_POOL_MAX_CHUNKS = 10000;         // 最大内存块数
    
    // 缓存配置
    constexpr size_t FILE_CACHE_SIZE = 1000;                 // 文件缓存条目数
    constexpr int FILE_CACHE_TTL_SECONDS = 300;              // 5分钟缓存TTL
    
    // 限流配置
    constexpr size_t MAX_REQUESTS_PER_SECOND = 10000;        // 每秒最大请求数
    constexpr size_t MAX_BYTES_PER_SECOND = 100 * 1024 * 1024; // 100MB/秒带宽限制
    
    // 日志配置
    constexpr bool ENABLE_PERFORMANCE_LOGGING = true;        // 启用性能日志
    constexpr bool ENABLE_DETAILED_METRICS = true;           // 启用详细指标
    
    // 平台特定配置
#ifdef _WIN32
    constexpr size_t IOCP_MAX_CONCURRENT_IO = 1000;          // IOCP最大并发I/O
    constexpr size_t IOCP_THREAD_POOL_SIZE = 16;             // IOCP线程池大小
#else
    constexpr size_t EPOLL_MAX_EVENTS = 10000;               // epoll最大事件数
    constexpr size_t EPOLL_TIMEOUT_MS = 100;                 // epoll超时时间
#endif
    
    // 性能等级配置
    enum class PerformanceLevel {
        LOW,        // 低性能 - 适合开发测试
        MEDIUM,     // 中等性能 - 适合小规模生产
        HIGH,       // 高性能 - 适合中等规模生产
        EXTREME     // 极限性能 - 适合大规模生产
    };
    
    // 根据性能等级获取配置
    struct PerformanceSettings {
        size_t max_connections;
        size_t thread_pool_size;
        size_t task_queue_size;
        size_t read_buffer_size;
        size_t write_buffer_size;
        int connection_timeout_ms;
        bool enable_compression;
        bool enable_keep_alive;
        bool enable_connection_pooling;
    };
    
    PerformanceSettings get_settings(PerformanceLevel level);
    
    // 性能监控指标
    struct PerformanceMetrics {
        // 连接统计
        std::atomic<size_t> active_connections{0};
        std::atomic<size_t> total_connections{0};
        std::atomic<size_t> rejected_connections{0};
        
        // 请求统计
        std::atomic<size_t> total_requests{0};
        std::atomic<size_t> successful_requests{0};
        std::atomic<size_t> failed_requests{0};
        
        // 吞吐量统计
        std::atomic<size_t> bytes_received{0};
        std::atomic<size_t> bytes_sent{0};
        std::atomic<double> requests_per_second{0.0};
        std::atomic<double> bytes_per_second{0.0};
        
        // 延迟统计
        std::atomic<double> average_response_time_ms{0.0};
        std::atomic<double> min_response_time_ms{0.0};
        std::atomic<double> max_response_time_ms{0.0};
        
        // 错误统计
        std::atomic<size_t> timeout_errors{0};
        std::atomic<size_t> connection_errors{0};
        std::atomic<size_t> protocol_errors{0};
        
        // 文件操作统计
        std::atomic<size_t> file_uploads{0};
        std::atomic<size_t> file_downloads{0};
        std::atomic<size_t> file_deletions{0};
        std::atomic<size_t> total_file_size{0};
        
        // 内存使用统计
        std::atomic<size_t> memory_usage_bytes{0};
        std::atomic<size_t> peak_memory_usage_bytes{0};
        std::atomic<size_t> buffer_pool_usage{0};
        
        // 时间戳
        std::chrono::steady_clock::time_point last_update;
        
        // 重置统计
        void reset();
        
        // 更新统计
        void update_metrics();
        
        // 获取性能报告
        std::string get_performance_report() const;
    };
    
    // 全局性能监控实例
    extern PerformanceMetrics global_metrics;
    
    // 性能监控工具类
    class PerformanceMonitor {
    public:
        static void start_monitoring();
        static void stop_monitoring();
        static void log_metric(const std::string& name, double value);
        static void log_event(const std::string& event);
        
    private:
        static std::thread monitoring_thread_;
        static std::atomic<bool> monitoring_active_;
    };
    
} // namespace PerformanceConfig

#endif // PERFORMANCE_CONFIG_H 