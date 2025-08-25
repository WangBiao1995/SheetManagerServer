#!/usr/bin/env python3
import socket
import time

def test_raw_connection():
    try:
        # 创建socket连接
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)  # 设置10秒超时
        
        print("连接到服务器...")
        sock.connect(('127.0.0.1', 8080))
        print("连接成功！")
        
        # 发送HTTP请求
        request = "GET /files HTTP/1.1\r\nHost: localhost:8080\r\nConnection: close\r\n\r\n"
        print(f"发送请求: {request}")
        sock.send(request.encode())
        
        # 接收响应
        print("等待响应...")
        response = b""
        start_time = time.time()
        
        while True:
            try:
                chunk = sock.recv(1024)
                if chunk:
                    response += chunk
                    print(f"收到数据块: {len(chunk)} 字节，总计: {len(response)} 字节")
                else:
                    print("连接关闭")
                    break
            except socket.timeout:
                print("接收超时")
                break
        
        print(f"总响应长度: {len(response)} 字节")
        print(f"响应前200字节: {response[:200]}")
        
        # 尝试解析HTTP响应
        try:
            response_text = response.decode('utf-8', errors='ignore')
            print("响应解析成功（忽略编码错误）")
            print(f"响应内容: {response_text[:500]}...")
        except Exception as e:
            print(f"响应解析失败: {e}")
        
        sock.close()
        print("测试完成")
        
    except Exception as e:
        print(f"错误: {e}")

if __name__ == "__main__":
    test_raw_connection() 