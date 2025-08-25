#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

#ifdef _WIN32
    // Windows系统 - 使用IOCP
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>
    typedef SOCKET socket_t;
    typedef int socklen_t;
    typedef HANDLE iocp_handle_t;
#else
    // Linux/macOS系统 - 使用epoll
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <sys/epoll.h>
    #include <fcntl.h>
    typedef int socket_t;
    typedef int epoll_handle_t;
#endif

// 前向声明
class Connection;
class TaskQueue;
struct HttpRequest;
struct HttpResponse;
class HttpHandler;

// 连接状态枚举
enum class ConnectionState {
    CONNECTING,
    READING,
    WRITING,
    CLOSING,
    CLOSED
};

// 异步任务类型
enum class TaskType {
    READ_REQUEST,
    WRITE_RESPONSE,
    FILE_UPLOAD,
    FILE_DOWNLOAD,
    FILE_DELETE
};

// 异步任务结构
struct AsyncTask {
    TaskType type;
    std::shared_ptr<Connection> connection;
    std::function<void()> callback;
    std::chrono::steady_clock::time_point timestamp;
    
    // 默认构造函数
    AsyncTask() : type(TaskType::READ_REQUEST), connection(nullptr), callback(nullptr), 
                  timestamp(std::chrono::steady_clock::now()) {}
    
    // 带参数的构造函数
    AsyncTask(TaskType t, std::shared_ptr<Connection> conn, std::function<void()> cb)
        : type(t), connection(conn), callback(cb), timestamp(std::chrono::steady_clock::now()) {}
};

class Server {
public:
    Server(int port = 8080, size_t max_connections = 1000, size_t thread_pool_size = 4);
    ~Server();
    
    bool start();
    void stop();
    bool is_running() const { return running_; }
    
    // 性能统计
    size_t get_active_connections() const;
    size_t get_total_requests() const;
    double get_requests_per_second() const;
    
    // 性能配置常量 - 移到public部分
    static const size_t DEFAULT_MAX_CONNECTIONS = 1000;
    static const size_t DEFAULT_THREAD_POOL_SIZE = 4;
    static const size_t BUFFER_SIZE = 64 * 1024;  // 64KB缓冲区
    static const int CONNECTION_TIMEOUT_MS = 30000;  // 30秒超时
    
private:
    void accept_connections();
    void worker_thread_loop();
    void handle_async_task(AsyncTask& task);
    void cleanup_expired_connections();
    
    // 连接管理
    void add_connection(std::shared_ptr<Connection> conn);
    void remove_connection(std::shared_ptr<Connection> conn);
    
    // 平台特定的I/O处理
#ifdef _WIN32
    bool setup_iocp();
    void handle_iocp_completion();
#else
    bool setup_epoll();
    void handle_epoll_events();
#endif
    
    int port_;
    socket_t server_socket_;
    std::atomic<bool> running_;
    std::thread accept_thread_;
    
    // 连接池管理
    std::atomic<size_t> active_connections_;
    std::atomic<size_t> total_requests_;
    std::mutex connections_mutex_;
    std::vector<std::shared_ptr<Connection>> connections_;
    
    // 线程池
    std::vector<std::thread> worker_threads_;
    std::unique_ptr<TaskQueue> task_queue_;
    
    // 性能配置
    size_t max_connections_;
    size_t thread_pool_size_;
    
#ifdef _WIN32
    iocp_handle_t iocp_handle_;
    std::thread iocp_thread_;  // 添加IOCP线程
#else
    epoll_handle_t epoll_handle_;
    std::vector<struct epoll_event> epoll_events_;
#endif
};

// 连接类
class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(socket_t socket, const std::string& client_ip);
    ~Connection();
    
    // 异步操作
    void async_read();
    void async_write(const std::string& data);
    void async_close();
    
    // 状态查询
    bool is_active() const { return state_ != ConnectionState::CLOSED; }
    ConnectionState get_state() const { return state_; }
    std::string get_client_ip() const { return client_ip_; }
    
    // 数据访问
    std::string& get_read_buffer() { return read_buffer_; }
    std::string& get_write_buffer() { return write_buffer_; }
    
    // 超时检查
    bool is_expired() const;
    void update_activity();
    
private:
    void set_state(ConnectionState state);
    void handle_read_completion(size_t bytes_read);
    void handle_write_completion(size_t bytes_written);
    
    socket_t socket_;
    std::string client_ip_;
    ConnectionState state_;
    
    // 缓冲区
    std::string read_buffer_;
    std::string write_buffer_;
    
    // 活动时间
    std::chrono::steady_clock::time_point last_activity_;
    
    // 平台特定的I/O状态
#ifdef _WIN32
    OVERLAPPED read_overlapped_;
    OVERLAPPED write_overlapped_;
    WSABUF read_wsabuf_;
    WSABUF write_wsabuf_;
#else
    bool non_blocking_;
#endif
};

// 任务队列类
class TaskQueue {
public:
    TaskQueue(size_t max_size = 10000);
    
    void push(AsyncTask task);
    bool pop(AsyncTask& task);
    bool empty() const;
    size_t size() const;
    
private:
    std::queue<AsyncTask> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    size_t max_size_;
};

#endif // SERVER_H 