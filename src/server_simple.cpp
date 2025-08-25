#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#include "../include/http_handler.h"
#include "../include/file_manager.h"

void handle_client(SOCKET client_socket, const std::string& client_ip) {
    std::cout << "处理来自 " << client_ip << " 的请求" << std::endl;
    
    // 读取HTTP请求
    char buffer[4096];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string request_data(buffer);
        
        std::cout << "收到请求: " << request_data.substr(0, 100) << "..." << std::endl;
        
        // 解析HTTP请求
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
        send(client_socket, response_data.c_str(), response_data.length(), 0);
    }
    
    closesocket(client_socket);
    std::cout << "连接已关闭" << std::endl;
}

int main() {
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup失败!" << std::endl;
        return 1;
    }
    
    std::cout << "Winsock初始化成功" << std::endl;
    
    // 创建socket
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "创建socket失败" << std::endl;
        WSACleanup();
        return 1;
    }
    
    // 设置socket选项
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
        std::cerr << "设置socket选项失败" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    
    // 绑定地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "绑定端口失败" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    
    // 监听连接
    if (listen(server_socket, 5) < 0) {
        std::cerr << "监听失败" << std::endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    
    std::cout << "简单同步服务器启动成功！监听端口8080" << std::endl;
    std::cout << "按 Ctrl+C 停止服务器" << std::endl;
    
    // 接受连接
    while (true) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "接受连接失败" << std::endl;
            continue;
        }
        
        // 获取客户端IP地址
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "接受来自 " << client_ip << ":" << ntohs(client_addr.sin_port) << " 的连接" << std::endl;
        
        // 在新线程中处理客户端请求
        std::thread client_thread(handle_client, client_socket, std::string(client_ip));
        client_thread.detach();
    }
    
    closesocket(server_socket);
    WSACleanup();
    return 0;
} 