#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iomanip>

#ifdef _WIN32
    // 在包含Windows头文件之前定义NOMINMAX，防止min/max宏冲突
    #define NOMINMAX
    // Windows系统
    #include <windows.h>
    #include <conio.h>
#else
    // Linux/macOS系统
    #include <signal.h>
    #include <unistd.h>
#endif

#include <filesystem>
#include "../include/server.h"
#include "../include/file_manager.h"
#include "../include/performance_config.h"

Server* g_server = nullptr;

#ifdef _WIN32
// Windows信号处理
BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        std::cout << "\n收到Ctrl+C信号，正在关闭服务器..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
        return TRUE;
    }
    return FALSE;
}
#else
// Linux/macOS信号处理
void signal_handler(int signum) {
    std::cout << "\n收到信号 " << signum << "，正在关闭服务器..." << std::endl;
    if (g_server) {
        g_server->stop();
    }
}
#endif

void test_file_operations() {
    std::cout << "\n=== 测试文件操作 ===" << std::endl;
    
    FileManager file_manager;
    
    // 硬编码测试文件路径
    std::string test_file_path = "D:\\Documents\\Working\\客户资料\\人才公寓 结构(2) - 副本.dwg";
    
    // 检查源文件是否存在
    if (!std::filesystem::exists(test_file_path)) {
        std::cout << "警告: 测试文件不存在: " << test_file_path << std::endl;
        std::cout << "请确保文件存在，或者修改代码中的文件路径" << std::endl;
        return;
    }
    
    std::cout << "测试文件: " << test_file_path << std::endl;
    
    // 读取源文件
    std::ifstream source_file(test_file_path, std::ios::binary);
    if (!source_file.is_open()) {
        std::cerr << "无法打开源文件: " << test_file_path << std::endl;
        return;
    }
    
    // 获取文件大小
    source_file.seekg(0, std::ios::end);
    size_t file_size = source_file.tellg();
    source_file.seekg(0, std::ios::beg);
    
    std::cout << "文件大小: " << file_size << " 字节" << std::endl;
    
    // 读取文件内容
    std::vector<char> file_content(file_size);
    source_file.read(file_content.data(), file_size);
    source_file.close();
    
    // 提取文件名
    std::filesystem::path path(test_file_path);
    std::string filename = path.filename().string();
    
    std::cout << "文件名: " << filename << std::endl;
    
    // 保存到服务器
    if (file_manager.save_file(filename, file_content.data(), file_size)) {
        std::cout << "✓ 文件上传成功" << std::endl;
        
        // 验证文件是否保存成功
        if (file_manager.file_exists(filename)) {
            std::cout << "✓ 文件验证成功" << std::endl;
            
            // 读取文件进行对比
            std::vector<char> saved_content = file_manager.read_file(filename);
            if (saved_content.size() == file_size) {
                std::cout << "✓ 文件大小一致" << std::endl;
                
                // 简单的内容对比（前100字节）
                size_t compare_size = (std::min)(size_t(100), file_size);
                bool content_match = true;
                for (size_t i = 0; i < compare_size; ++i) {
                    if (file_content[i] != saved_content[i]) {
                        content_match = false;
                        break;
                    }
                }
                
                if (content_match) {
                    std::cout << "✓ 文件内容验证成功（前100字节）" << std::endl;
                } else {
                    std::cout << "✗ 文件内容验证失败" << std::endl;
                }
            } else {
                std::cout << "✗ 文件大小不一致: 原始=" << file_size << ", 保存=" << saved_content.size() << std::endl;
            }
        } else {
            std::cout << "✗ 文件验证失败" << std::endl;
        }
    } else {
        std::cout << "✗ 文件上传失败" << std::endl;
    }
    
    // 显示文件列表
    std::cout << "\n当前服务器文件列表:" << std::endl;
    std::vector<FileInfo> files = file_manager.list_files();
    if (files.empty()) {
        std::cout << "  暂无文件" << std::endl;
    } else {
        for (const auto& file : files) {
            std::cout << "  - " << file.filename << " (" << file.size << " 字节, " << file.last_modified << ")" << std::endl;
        }
    }
}

void print_performance_info() {
    std::cout << "\n=== 性能配置信息 ===" << std::endl;
    std::cout << "默认最大连接数: " << PerformanceConfig::DEFAULT_MAX_CONNECTIONS << std::endl;
    std::cout << "默认线程池大小: " << PerformanceConfig::DEFAULT_THREAD_POOL_SIZE << std::endl;
    std::cout << "默认任务队列大小: " << PerformanceConfig::DEFAULT_TASK_QUEUE_SIZE << std::endl;
    std::cout << "读取缓冲区大小: " << PerformanceConfig::DEFAULT_READ_BUFFER_SIZE << " 字节" << std::endl;
    std::cout << "写入缓冲区大小: " << PerformanceConfig::DEFAULT_WRITE_BUFFER_SIZE << " 字节" << std::endl;
    std::cout << "最大上传大小: " << PerformanceConfig::MAX_UPLOAD_SIZE / (1024 * 1024) << " MB" << std::endl;
    std::cout << "连接超时: " << PerformanceConfig::CONNECTION_TIMEOUT_MS << " ms" << std::endl;
    std::cout << "请求超时: " << PerformanceConfig::REQUEST_TIMEOUT_MS << " ms" << std::endl;
    
#ifdef _WIN32
    std::cout << "IOCP最大并发I/O: " << PerformanceConfig::IOCP_MAX_CONCURRENT_IO << std::endl;
    std::cout << "IOCP线程池大小: " << PerformanceConfig::IOCP_THREAD_POOL_SIZE << std::endl;
#else
    std::cout << "epoll最大事件数: " << PerformanceConfig::EPOLL_MAX_EVENTS << std::endl;
    std::cout << "epoll超时时间: " << PerformanceConfig::EPOLL_TIMEOUT_MS << " ms" << std::endl;
#endif
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // 初始化Windows Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock初始化失败!" << std::endl;
        return 1;
    }
    std::cout << "Winsock初始化成功" << std::endl;
#endif

    int port = 8080;
    std::string address = "127.0.0.1";
    size_t max_connections = PerformanceConfig::DEFAULT_MAX_CONNECTIONS;
    size_t thread_pool_size = PerformanceConfig::DEFAULT_THREAD_POOL_SIZE;
    
    // 解析命令行参数
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    if (argc > 2) {
        max_connections = std::stoul(argv[2]);
    }
    if (argc > 3) {
        thread_pool_size = std::stoul(argv[3]);
    }
    
    std::cout << "🚀 启动高性能异步文件服务器" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "端口: " << port << std::endl;
    std::cout << "最大连接数: " << max_connections << std::endl;
    std::cout << "工作线程数: " << thread_pool_size << std::endl;
    
    print_performance_info();
    
    std::cout << "\n支持的功能:" << std::endl;
    std::cout << "  - 文件上传: POST /upload" << std::endl;
    std::cout << "  - 文件下载: GET /download/{filename}" << std::endl;
    std::cout << "  - 文件列表: GET /files" << std::endl;
    std::cout << "  - 删除文件: DELETE /delete/{filename}" << std::endl;
    std::cout << "  - 性能监控: GET /stats" << std::endl;
    
    std::cout << "\n按 Ctrl+C 停止服务器" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // 测试文件操作
   // test_file_operations();
    
    // 设置信号处理器
#ifdef _WIN32
    SetConsoleCtrlHandler(console_handler, TRUE);
#else
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
#endif
    
    // 创建并启动高性能服务器
    Server server(address,port, max_connections, thread_pool_size);
    g_server = &server;
    
    if (!server.start()) {
        std::cerr << "启动服务器失败!" << std::endl;
        return 1;
    }
    
    // 性能监控线程
    std::thread stats_thread([&server]() {
        while (server.is_running()) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            std::cout << "\n📊 性能统计:" << std::endl;
            std::cout << "  活跃连接: " << server.get_active_connections() << std::endl;
            std::cout << "  总请求数: " << server.get_total_requests() << std::endl;
            std::cout << "  请求/秒: " << std::fixed << std::setprecision(2) 
                      << server.get_requests_per_second() << std::endl;
        }
    });
    
    // 等待服务器运行
    while (server.is_running()) {
#ifdef _WIN32
        std::this_thread::sleep_for(std::chrono::seconds(1));
#else
        sleep(1);
#endif
    }
    
    // 等待统计线程结束
    if (stats_thread.joinable()) {
        stats_thread.join();
    }
    
    std::cout << "\n服务器已关闭" << std::endl;
    std::cout << "最终统计:" << std::endl;
    std::cout << "  总连接数: " << server.get_total_requests() << std::endl;
    
#ifdef _WIN32
    // 清理Winsock
    WSACleanup();
    std::cout << "Winsock已清理" << std::endl;
#endif
    
    return 0;
} 