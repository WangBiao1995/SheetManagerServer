#include "../include/server.h"
#include "../include/http_handler.h"
#include "../include/file_manager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    // Windows系统 - 使用IOCP
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>
    // 定义socket关闭函数别名，避免与POSIX close冲突
    #define socket_close closesocket
    #define socklen_t int
#else
    // Linux/macOS系统 - 使用epoll
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <errno.h>
    #define socket_close close
#endif

// TaskQueue 实现
TaskQueue::TaskQueue(size_t max_size) : max_size_(max_size) {}

void TaskQueue::push(AsyncTask task) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (tasks_.size() < max_size_) {
        tasks_.push(std::move(task));
        not_empty_.notify_one();
    }
}

bool TaskQueue::pop(AsyncTask& task) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_.wait(lock, [this] { return !tasks_.empty(); });
    
    if (!tasks_.empty()) {
        task = std::move(tasks_.front());
        tasks_.pop();
        return true;
    }
    return false;
}

bool TaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.empty();
}

size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

// Connection 实现
Connection::Connection(socket_t socket, const std::string& client_ip) 
    : socket_(socket), client_ip_(client_ip), state_(ConnectionState::CONNECTING) {
    
    last_activity_ = std::chrono::steady_clock::now();
    
#ifdef _WIN32
    // 初始化Windows异步I/O结构
    ZeroMemory(&read_overlapped_, sizeof(OVERLAPPED));
    ZeroMemory(&write_overlapped_, sizeof(OVERLAPPED));
    
    read_wsabuf_.buf = nullptr;
    read_wsabuf_.len = 0;
    write_wsabuf_.buf = nullptr;
    write_wsabuf_.len = 0;
#else
    // 设置非阻塞模式
    int flags = fcntl(socket_, F_GETFL, 0);
    fcntl(socket_, F_SETFL, flags | O_NONBLOCK);
    non_blocking_ = true;
#endif
}

Connection::~Connection() {
    if (socket_ != INVALID_SOCKET) {
        socket_close(socket_);
    }
}

void Connection::set_state(ConnectionState state) {
    state_ = state;
    update_activity();
}

void Connection::async_read() {
    if (state_ == ConnectionState::CLOSED) return;
    
    set_state(ConnectionState::READING);
    std::cout << "开始读取，连接状态: " << static_cast<int>(state_) << std::endl;
    
#ifdef _WIN32
    // 临时使用同步读取进行测试
    read_buffer_.resize(Server::BUFFER_SIZE);
    DWORD bytes_received = recv(socket_, &read_buffer_[0], read_buffer_.size(), 0);
    
    if (bytes_received > 0) {
        std::cout << "同步读取完成，收到 " << bytes_received << " 字节" << std::endl;
        handle_read_completion(bytes_received);
    } else if (bytes_received == 0) {
        std::cout << "客户端关闭连接" << std::endl;
        // 延迟关闭连接，给客户端时间处理响应
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        set_state(ConnectionState::CLOSING);
    } else {
        int error = WSAGetLastError();
        if (error == WSAECONNRESET || error == WSAECONNABORTED) {
            std::cout << "连接被客户端重置，错误码: " << error << std::endl;
        } else {
            std::cerr << "读取失败，错误码: " << error << std::endl;
        }
        set_state(ConnectionState::CLOSING);
    }
#else
    // Linux epoll非阻塞读取
    read_buffer_.resize(Server::BUFFER_SIZE);
    ssize_t bytes_received = recv(socket_, &read_buffer_[0], read_buffer_.size(), 0);
    
    if (bytes_received > 0) {
        read_buffer_.resize(bytes_received);
        handle_read_completion(bytes_received);
    } else if (bytes_received == 0) {
        set_state(ConnectionState::CLOSING);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        set_state(ConnectionState::CLOSING);
    }
#endif
}

void Connection::async_write(const std::string& data) {
    if (state_ == ConnectionState::CLOSED) return;
    
    set_state(ConnectionState::WRITING);
    write_buffer_ = data;
    std::cout << "开始发送响应，数据长度: " << data.length() << " 字节" << std::endl;
    
#ifdef _WIN32
    // 使用同步发送确保数据完全发送
    const char* buffer = data.data();
    size_t total_sent = 0;
    size_t remaining = data.length();
    
    while (remaining > 0) {
        int bytes_sent = send(socket_, buffer + total_sent, static_cast<int>(remaining), 0);
        
        if (bytes_sent > 0) {
            total_sent += bytes_sent;
            remaining -= bytes_sent;
            std::cout << "已发送 " << total_sent << "/" << data.length() << " 字节" << std::endl;
        } else if (bytes_sent == 0) {
            std::cout << "连接关闭，无法发送更多数据" << std::endl;
            break;
        } else {
            int error = WSAGetLastError();
            std::cout << "发送失败，错误码: " << error << std::endl;
            break;
        }
    }
    
    if (total_sent == data.length()) {
        std::cout << "响应完全发送成功！" << std::endl;
        
        // 等待数据完全发送到网络
        std::cout << "等待数据发送到网络..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        handle_write_completion(total_sent);
    } else {
        std::cout << "响应发送不完整，已发送 " << total_sent << "/" << data.length() << " 字节" << std::endl;
        set_state(ConnectionState::CLOSING);
    }
#else
    // Linux epoll非阻塞写入
    ssize_t bytes_sent = send(socket_, write_buffer_.data(), write_buffer_.length(), 0);
    
    if (bytes_sent > 0) {
        write_buffer_.erase(0, bytes_sent);
        if (write_buffer_.empty()) {
            set_state(ConnectionState::READING);
        }
    } else if (bytes_sent == 0) {
        set_state(ConnectionState::CLOSING);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        set_state(ConnectionState::CLOSING);
    }
#endif
}

void Connection::async_close() {
    set_state(ConnectionState::CLOSING);
}

bool Connection::is_expired() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_activity_);
    return elapsed.count() > Server::CONNECTION_TIMEOUT_MS;
}

void Connection::update_activity() {
    last_activity_ = std::chrono::steady_clock::now();
}

void Connection::handle_read_completion(size_t bytes_read) {
    if (bytes_read > 0) {
        read_buffer_.resize(bytes_read);
        update_activity();
        
        std::cout << "收到 " << bytes_read << " 字节数据" << std::endl;
        
        // 解析HTTP请求
        std::string request_data(read_buffer_.begin(), read_buffer_.end());
        std::cout << "HTTP请求数据: " << request_data.substr(0, 100) << "..." << std::endl;
        
        HttpHandler http_handler;
        HttpRequest request = http_handler.parse_request(request_data);
        
        std::cout << "解析的请求: " << request.method << " " << request.path << std::endl;
        
        // 处理HTTP请求
        HttpResponse response;
        if (request.method == "GET") {
            if (request.path == "/files") {
                std::cout << "处理文件列表请求" << std::endl;
                response = http_handler.handle_list_files(request);
            } else if (request.path.substr(0, 10) == "/download/") {
                std::cout << "处理文件下载请求: " << request.path << std::endl;
                response = http_handler.handle_download(request);
            } else if (request.path == "/stats") {
                std::cout << "处理性能统计请求" << std::endl;
                // 返回性能统计信息
                response.status_code = 200;
                response.status_text = "OK";
                response.headers["Content-Type"] = "application/json";
                response.body = "{\"status\":\"success\",\"message\":\"性能统计信息\"}";
            } else {
                std::cout << "404 Not Found: " << request.path << std::endl;
                response.status_code = 404;
                response.status_text = "Not Found";
                response.headers["Content-Type"] = "text/plain";
                response.body = "404 Not Found";
            }
        } else if (request.method == "POST") {
            if (request.path == "/upload") {
                std::cout << "处理文件上传请求" << std::endl;
                response = http_handler.handle_upload(request);
            } else {
                std::cout << "404 Not Found: " << request.path << std::endl;
                response.status_code = 404;
                response.status_text = "Not Found";
                response.headers["Content-Type"] = "text/plain";
                response.body = "404 Not Found";
            }
        } else if (request.method == "DELETE") {
            if (request.path.substr(0, 8) == "/delete/") {
                std::cout << "处理文件删除请求: " << request.path << std::endl;
                response = http_handler.handle_delete_file(request);
            } else {
                std::cout << "404 Not Found: " << request.path << std::endl;
                response.status_code = 404;
                response.status_text = "Not Found";
                response.headers["Content-Type"] = "text/plain";
                response.body = "404 Not Found";
            }
        } else {
            std::cout << "405 Method Not Allowed: " << request.method << std::endl;
            response.status_code = 405;
            response.status_text = "Method Not Allowed";
            response.headers["Content-Type"] = "text/plain";
            response.body = "405 Method Not Allowed";
        }
        
        std::cout << "响应状态: " << response.status_code << " " << response.status_text << std::endl;
        
        // 构建HTTP响应
        std::string response_data = http_handler.build_response(response);
        
        std::cout << "发送响应，长度: " << response_data.length() << " 字节" << std::endl;
        
        // 发送响应
        async_write(response_data);
        
        // 清空读取缓冲区，准备下一次读取
        read_buffer_.clear();
        
        // 不要在这里立即调用async_read，等待写入完成后再调用
        // set_state(ConnectionState::READING);
        // async_read();
    } else {
        std::cout << "读取完成，字节数为0，关闭连接" << std::endl;
        set_state(ConnectionState::CLOSING);
    }
}

void Connection::handle_write_completion(size_t bytes_written) {
    if (bytes_written > 0) {
        write_buffer_.erase(0, bytes_written);
        update_activity();
        
        std::cout << "写入完成回调: 已写入 " << bytes_written << " 字节" << std::endl;
        
        if (write_buffer_.empty()) {
            std::cout << "响应发送完成，准备关闭连接..." << std::endl;
            
            // 等待数据完全发送到网络
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // 关闭连接
            std::cout << "关闭连接..." << std::endl;
            set_state(ConnectionState::CLOSING);
            
            // 发送shutdown信号
            shutdown(socket_, SD_SEND);
            
            // 关闭socket
            socket_close(socket_);
            set_state(ConnectionState::CLOSED);
        }
    } else {
        set_state(ConnectionState::CLOSING);
    }
}

// Server 实现
Server::Server(int port, size_t max_connections, size_t thread_pool_size)
    : port_(port), server_socket_(INVALID_SOCKET), running_(false), 
      active_connections_(0), total_requests_(0),
      max_connections_(max_connections), thread_pool_size_(thread_pool_size) {
    
    // 注意：WSAStartup在main函数中已经调用，这里不需要重复调用
    
    task_queue_ = std::make_unique<TaskQueue>();
}

Server::~Server() {
    stop();
    // 注意：WSACleanup在main函数中已经调用，这里不需要重复调用
}

bool Server::start() {
    // 创建socket
#ifdef _WIN32
    server_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
#endif

    if (server_socket_ == INVALID_SOCKET) {
        std::cerr << "创建socket失败" << std::endl;
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "设置socket选项失败" << std::endl;
        socket_close(server_socket_);
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "绑定端口失败" << std::endl;
        socket_close(server_socket_);
        return false;
    }
    
    // 监听连接
    if (listen(server_socket_, static_cast<int>(max_connections_)) < 0) {
        std::cerr << "监听失败" << std::endl;
        socket_close(server_socket_);
        return false;
    }
    
    // 设置平台特定的I/O
#ifdef _WIN32
    if (!setup_iocp()) {
        return false;
    }
#else
    if (!setup_epoll()) {
        return false;
    }
#endif
    
    std::cout << "高性能异步服务器启动成功！" << std::endl;
    std::cout << "监听端口: " << port_ << std::endl;
    std::cout << "最大连接数: " << max_connections_ << std::endl;
    std::cout << "工作线程数: " << thread_pool_size_ << std::endl;
    
    running_ = true;
    
    // 启动工作线程池
    for (size_t i = 0; i < thread_pool_size_; ++i) {
        worker_threads_.emplace_back(&Server::worker_thread_loop, this);
    }
    
    // 启动IOCP完成处理线程
    iocp_thread_ = std::thread(&Server::handle_iocp_completion, this);
    
    // 启动接受连接线程
    accept_thread_ = std::thread(&Server::accept_connections, this);
    
    return true;
}

void Server::stop() {
    if (!running_) return;
    
    running_ = false;
    
    // 关闭socket
    if (server_socket_ != INVALID_SOCKET) {
        socket_close(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }
    
    // 等待工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // 等待IOCP线程结束
    if (iocp_thread_.joinable()) {
        iocp_thread_.join();
    }
    
    // 等待接受线程结束
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
    
    std::cout << "服务器已停止" << std::endl;
}

void Server::accept_connections() {
    std::cout << "开始接受连接线程..." << std::endl;
    
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        std::cout << "等待新连接..." << std::endl;
        
        socket_t client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            if (running_) {
                int error = WSAGetLastError();
                std::cerr << "接受连接失败，错误码: " << error << std::endl;
            }
            continue;
        }
        
        std::cout << "成功接受新连接，socket: " << client_socket << std::endl;
        
        // 检查连接数限制
        if (active_connections_ >= max_connections_) {
            std::cout << "达到最大连接数限制，拒绝新连接" << std::endl;
            socket_close(client_socket);
            continue;
        }
        
        // 获取客户端IP地址
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "客户端IP: " << client_ip << ", 端口: " << ntohs(client_addr.sin_port) << std::endl;
        
        // 将客户端socket关联到IOCP
        if (CreateIoCompletionPort((HANDLE)client_socket, iocp_handle_, (ULONG_PTR)client_socket, 0) == nullptr) {
            std::cerr << "关联客户端socket到IOCP失败，错误码: " << GetLastError() << std::endl;
            socket_close(client_socket);
            continue;
        }
        
        // 创建新连接
        auto connection = std::make_shared<Connection>(client_socket, client_ip);
        add_connection(connection);
        
        std::cout << "接受来自 " << client_ip << ":" << ntohs(client_addr.sin_port) 
                  << " 的连接 (活跃连接: " << active_connections_ << ")" << std::endl;
        
        // 延迟一下再开始异步读取，确保连接完全建立
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        connection->async_read();
    }
}

void Server::worker_thread_loop() {
    while (running_) {
        AsyncTask task;
        if (task_queue_->pop(task)) {
            handle_async_task(task);
        }
        
        // 清理过期连接
        cleanup_expired_connections();
    }
}

void Server::handle_async_task(AsyncTask& task) {
    if (!task.connection || !task.connection->is_active()) {
        return;
    }
    
    try {
        if (task.callback) {
            task.callback();
        }
        total_requests_++;
    } catch (const std::exception& e) {
        std::cerr << "处理异步任务时发生异常: " << e.what() << std::endl;
    }
}

void Server::cleanup_expired_connections() {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    auto it = connections_.begin();
    while (it != connections_.end()) {
        if (!(*it)->is_active() || (*it)->is_expired()) {
            std::cout << "清理过期连接: " << (*it)->get_client_ip() << std::endl;
            it = connections_.erase(it);
            active_connections_--;
        } else {
            ++it;
        }
    }
}

void Server::add_connection(std::shared_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.push_back(conn);
    active_connections_++;
}

void Server::remove_connection(std::shared_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = std::find(connections_.begin(), connections_.end(), conn);
    if (it != connections_.end()) {
        connections_.erase(it);
        active_connections_--;
    }
}

#ifdef _WIN32
bool Server::setup_iocp() {
    iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (iocp_handle_ == nullptr) {
        std::cerr << "创建IOCP失败" << std::endl;
        return false;
    }
    
    // 将服务器socket关联到IOCP
    if (CreateIoCompletionPort((HANDLE)server_socket_, iocp_handle_, 0, 0) == nullptr) {
        std::cerr << "关联服务器socket到IOCP失败" << std::endl;
        return false;
    }
    
    return true;
}

void Server::handle_iocp_completion() {
    std::cout << "IOCP完成处理线程启动..." << std::endl;
    
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    OVERLAPPED* overlapped;
    
    while (running_) {
        BOOL result = GetQueuedCompletionStatus(iocp_handle_, &bytes_transferred, 
                                               &completion_key, &overlapped, 100);
        
        if (result) {
            // 处理I/O完成
            if (overlapped) {
                // 这里需要根据overlapped类型调用相应的完成处理函数
                // 暂时先输出调试信息
                std::cout << "IOCP完成: 传输字节数=" << bytes_transferred 
                          << ", 完成键=" << completion_key << std::endl;
            }
        } else {
            DWORD error = GetLastError();
            if (error != WAIT_TIMEOUT) {
                std::cerr << "IOCP等待失败，错误码: " << error << std::endl;
            }
        }
    }
    
    std::cout << "IOCP完成处理线程退出" << std::endl;
}
#else
bool Server::setup_epoll() {
    epoll_handle_ = epoll_create1(0);
    if (epoll_handle_ == -1) {
        std::cerr << "创建epoll失败" << std::endl;
        return false;
    }
    
    // 将服务器socket添加到epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = nullptr;
    
    if (epoll_ctl(epoll_handle_, EPOLL_CTL_ADD, server_socket_, &event) == -1) {
        std::cerr << "添加服务器socket到epoll失败" << std::endl;
        return false;
    }
    
    epoll_events_.resize(1000);  // 预分配事件数组
    return true;
}

void Server::handle_epoll_events() {
    while (running_) {
        int num_events = epoll_wait(epoll_handle_, epoll_events_.data(), 
                                   static_cast<int>(epoll_events_.size()), 100);
        
        if (num_events > 0) {
            for (int i = 0; i < num_events; ++i) {
                // 处理epoll事件
                if (epoll_events_[i].events & EPOLLIN) {
                    // 处理读取事件
                }
                if (epoll_events_[i].events & EPOLLOUT) {
                    // 处理写入事件
                }
            }
        }
    }
}
#endif

// 性能统计方法
size_t Server::get_active_connections() const {
    return active_connections_;
}

size_t Server::get_total_requests() const {
    return total_requests_;
}

double Server::get_requests_per_second() const {
    // 这里可以实现更复杂的统计逻辑
    static auto last_time = std::chrono::steady_clock::now();
    static size_t last_requests = 0;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();
    
    if (elapsed > 0) {
        size_t current_requests = total_requests_;
        size_t requests_diff = current_requests - last_requests;
        double rps = static_cast<double>(requests_diff) / elapsed;
        
        last_time = now;
        last_requests = current_requests;
        
        return rps;
    }
    
    return 0.0;
} 