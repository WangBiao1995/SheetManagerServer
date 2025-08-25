#!/usr/bin/env python3
import socket

def test_raw_socket():
    try:
        # 创建原始socket连接
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        
        print("连接到服务器...")
        sock.connect(('127.0.0.1', 8080))
        print("连接成功！")
        
        # 发送HTTP请求
        request = "GET /files HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"
        print(f"发送请求: {request}")
        sock.send(request.encode())
        
        # 接收响应
        print("等待响应...")
        response = sock.recv(1024)
        print(f"收到响应: {response.decode()}")
        
        sock.close()
        print("测试完成")
        
    except Exception as e:
        print(f"错误: {e}")

if __name__ == "__main__":
    test_raw_socket() 