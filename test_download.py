#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文件下载测试脚本
测试服务器的文件下载功能
"""

import requests
import os
import time
import json
from urllib.parse import quote, unquote

class DownloadTester:
    def __init__(self, base_url="http://localhost:8080"):
        self.base_url = base_url
        self.session = requests.Session()
        self.test_results = []
        
    def test_server_connection(self):
        """测试服务器连接"""
        try:
            response = self.session.get(f"{self.base_url}/files", timeout=5)
            if response.status_code == 200:
                print("✅ 服务器连接成功")
                return True
            else:
                print(f"❌ 服务器响应异常: {response.status_code}")
                return False
        except requests.exceptions.RequestException as e:
            print(f"❌ 无法连接到服务器: {e}")
            return False
    
    def get_file_list(self):
        """获取文件列表"""
        try:
            response = self.session.get(f"{self.base_url}/files")
            if response.status_code == 200:
                files_data = response.json()
                if files_data.get("status") == "success":
                    files = files_data.get("files", [])
                    print(f"📁 发现 {len(files)} 个文件:")
                    for file_info in files:
                        print(f"   - {file_info['filename']} ({file_info['size']} 字节)")
                    return files
                else:
                    print("❌ 获取文件列表失败")
                    return []
            else:
                print(f"❌ 获取文件列表失败: {response.status_code}")
                return []
        except Exception as e:
            print(f"❌ 获取文件列表异常: {e}")
            return []
    
    def test_download_file(self, filename, download_url):
        """测试单个文件下载"""
        print(f"\n🔄 测试下载文件: {filename}")
        
        try:
            # 开始计时
            start_time = time.time()
            
            # 发送下载请求
            response = self.session.get(f"{self.base_url}{download_url}", stream=True)
            
            if response.status_code == 200:
                # 检查Content-Disposition头
                content_disposition = response.headers.get('Content-Disposition', '')
                content_length = response.headers.get('Content-Length', '0')
                content_type = response.headers.get('Content-Type', '')
                
                print(f"   📥 下载成功")
                print(f"   📊 文件大小: {content_length} 字节")
                print(f"   🏷️  Content-Type: {content_type}")
                print(f"   📎 Content-Disposition: {content_disposition}")
                
                # 从Content-Disposition头中解析实际文件名
                actual_filename = self.parse_filename_from_disposition(content_disposition)
                if not actual_filename:
                    # 如果解析失败，使用原始文件名
                    actual_filename = filename
                
                print(f"   📝 解析到的文件名: {actual_filename}")
                
                # 保存文件到测试目录
                test_dir = "download_test_files"
                os.makedirs(test_dir, exist_ok=True)
                
                # 使用解析到的文件名，而不是原始文件名
                test_filename = f"test_download_{actual_filename}"
                
                # 确保文件名是有效的Windows文件名
                test_filename = self.sanitize_filename(test_filename)
                
                file_path = os.path.join(test_dir, test_filename)
                print(f"   🔧 清理后的文件名: {test_filename}")
                
                with open(file_path, 'wb') as f:
                    downloaded_size = 0
                    for chunk in response.iter_content(chunk_size=8192):
                        if chunk:
                            f.write(chunk)
                            downloaded_size += len(chunk)
                
                # 计算下载时间
                end_time = time.time()
                download_time = end_time - start_time
                download_speed = int(downloaded_size / download_time) if download_time > 0 else 0
                
                print(f"   ⏱️  下载时间: {download_time:.2f} 秒")
                print(f"   🚀 下载速度: {download_speed} 字节/秒")
                print(f"   💾 保存到: {file_path}")
                
                # 验证文件大小
                if os.path.exists(file_path):
                    actual_size = os.path.getsize(file_path)
                    if int(content_length) == actual_size:
                        print(f"   ✅ 文件大小验证成功")
                        result = {
                            "filename": filename,
                            "status": "success",
                            "size": actual_size,
                            "download_time": download_time,
                            "download_speed": download_speed
                        }
                    else:
                        print(f"   ⚠️  文件大小不匹配: 期望 {content_length}, 实际 {actual_size}")
                        result = {
                            "filename": filename,
                            "status": "size_mismatch",
                            "expected_size": int(content_length),
                            "actual_size": actual_size
                        }
                else:
                    print(f"   ❌ 文件保存失败")
                    result = {
                        "filename": filename,
                        "status": "save_failed"
                    }
                
            else:
                print(f"   ❌ 下载失败: HTTP {response.status_code}")
                result = {
                    "filename": filename,
                    "status": "http_error",
                    "status_code": response.status_code
                }
                
        except Exception as e:
            print(f"   ❌ 下载异常: {e}")
            result = {
                "filename": filename,
                "status": "exception",
                "error": str(e)
            }
        
        self.test_results.append(result)
        return result
    
    def manual_url_decode(self, encoded_str):
        """手动URL解码，专门处理中文文件名"""
        try:
            # 先进行URL解码，得到字节序列
            decoded_bytes = bytearray()
            i = 0
            while i < len(encoded_str):
                if encoded_str[i] == '%' and i + 2 < len(encoded_str):
                    # 提取十六进制值
                    hex_str = encoded_str[i+1:i+3]
                    try:
                        # 转换为整数
                        char_code = int(hex_str, 16)
                        decoded_bytes.append(char_code)
                        i += 3
                    except ValueError:
                        # 如果转换失败，保持原样
                        decoded_bytes.append(ord(encoded_str[i]))
                        i += 1
                elif encoded_str[i] == '+':
                    decoded_bytes.append(32)  # 空格
                    i += 1
                else:
                    decoded_bytes.append(ord(encoded_str[i]))
                    i += 1
            
            # 现在尝试用不同的编码来解码字节序列
            print(f"   🔍 解码后的字节: {decoded_bytes}")
            
            # 尝试UTF-8解码
            try:
                result_utf8 = decoded_bytes.decode('utf-8')
                print(f"   🔍 UTF-8解码: {result_utf8}")
                if self.is_valid_chinese_text(result_utf8):
                    return result_utf8
            except UnicodeDecodeError:
                print("   ⚠️  UTF-8解码失败")
            
            # 尝试GBK解码（Windows中文系统常用）
            try:
                result_gbk = decoded_bytes.decode('gbk')
                print(f"    GBK解码: {result_gbk}")
                if self.is_valid_chinese_text(result_gbk):
                    return result_gbk
            except UnicodeDecodeError:
                print("   ⚠️  GBK解码失败")
            
            # 尝试GB2312解码
            try:
                result_gb2312 = decoded_bytes.decode('gb2312')
                print(f"   🔍 GB2312解码: {result_gb2312}")
                if self.is_valid_chinese_text(result_gb2312):
                    return result_gb2312
            except UnicodeDecodeError:
                print("   ⚠️  GB2312解码失败")
            
            # 尝试ISO-8859-1解码（Latin-1）
            try:
                result_latin1 = decoded_bytes.decode('latin-1')
                print(f"    Latin-1解码: {result_latin1}")
                return result_latin1
            except UnicodeDecodeError:
                print("   ⚠️  Latin-1解码失败")
            
            # 如果所有解码都失败，返回原始字符串
            print("   ⚠️  所有解码方法都失败，返回原始字符串")
            return encoded_str
            
        except Exception as e:
            print(f"   ⚠️  手动解码异常: {e}")
            return encoded_str
    
    def is_valid_chinese_text(self, text):
        """检查文本是否是有效的中文文本"""
        if not text:
            return False
        
        # 检查是否包含中文字符
        chinese_chars = 0
        total_chars = len(text)
        
        for char in text:
            # 中文字符的Unicode范围
            if '\u4e00' <= char <= '\u9fff':  # 基本汉字
                chinese_chars += 1
            elif '\u3400' <= char <= '\u4dbf':  # 扩展A区
                chinese_chars += 1
            elif '\u20000' <= char <= '\u2a6df':  # 扩展B区
                chinese_chars += 1
        
        # 如果包含中文字符，认为是有效的中文文本
        return chinese_chars > 0
    
    def parse_filename_from_disposition(self, content_disposition):
        """从Content-Disposition头中解析文件名"""
        if not content_disposition:
            print("   ⚠️  Content-Disposition头为空")
            return None
        
        print(f"   🔍 解析Content-Disposition: '{content_disposition}'")
        
        try:
            # 查找filename=部分
            filename_start = content_disposition.find('filename=')
            if filename_start == -1:
                print("   ⚠️  未找到filename=部分")
                return None
            
            # 跳过"filename="
            filename_start += 9
            
            # 检查是否有引号
            if filename_start < len(content_disposition) and content_disposition[filename_start] == '"':
                filename_start += 1
                filename_end = content_disposition.find('"', filename_start)
                if filename_end == -1:
                    print("   ⚠️  引号不匹配")
                    return None
            else:
                # 没有引号，查找分号或行尾
                filename_end = content_disposition.find(';', filename_start)
                if filename_end == -1:
                    filename_end = len(content_disposition)
            
            # 提取文件名
            filename = content_disposition[filename_start:filename_end].strip()
            
            # 直接使用手动解码，跳过有问题的urllib.parse.unquote
            decoded_filename = self.manual_url_decode(filename)
            
            # 验证解码结果
            if decoded_filename and decoded_filename != filename:
                print(f"   ✅ 手动解码成功: '{filename}' -> '{decoded_filename}'")
                return decoded_filename
            else:
                print(f"   ⚠️  手动解码失败，使用原始文件名")
                return filename
                
        except Exception as e:
            print(f"   ⚠️  解析Content-Disposition失败: {e}")
            return None
    
    def is_garbled_text(self, text):
        """检查文本是否是乱码"""
        if not text:
            return False
        
        # 检查是否包含大量乱码字符
        garbled_chars = 0
        total_chars = len(text)
        
        for char in text:
            # 检查是否是常见的乱码字符
            if ord(char) < 32 and char not in '\t\n\r':
                garbled_chars += 1
            elif ord(char) > 127 and ord(char) < 160:
                garbled_chars += 1
        
        # 如果超过30%的字符是乱码，认为是乱码文本
        return (garbled_chars / total_chars) > 0.3
    
    def test_special_characters(self):
        """测试特殊字符文件名下载"""
        print("\n🔍 测试特殊字符文件名下载")
        
        # 测试包含特殊字符的文件名
        special_files = [
            "file with spaces.txt",
            "中文文件名.txt",
            "file@domain.com.txt"
        ]
        
        for filename in special_files:
            # URL编码文件名
            encoded_filename = quote(filename)
            download_url = f"/download/{encoded_filename}"
            
            print(f"\n🔄 测试特殊字符文件: {filename}")
            print(f"   🔗 编码后URL: {download_url}")
            
            try:
                response = self.session.get(f"{self.base_url}{download_url}", stream=True)
                
                if response.status_code == 200:
                    print(f"   ✅ 特殊字符文件下载成功")
                    # 验证Content-Disposition中的文件名
                    content_disposition = response.headers.get('Content-Disposition', '')
                    if filename in content_disposition:
                        print(f"   ✅ 文件名编码正确")
                    else:
                        print(f"   ⚠️  文件名编码可能有问题")
                elif response.status_code == 404:
                    print(f"   ℹ️  文件不存在 (这是正常的)")
                else:
                    print(f"   ❌ 下载失败: HTTP {response.status_code}")
                    
            except Exception as e:
                print(f"   ❌ 测试异常: {e}")
    
    def run_all_tests(self):
        """运行所有测试"""
        print("�� 开始文件下载测试")
        print("=" * 50)
        
        # 测试服务器连接
        if not self.test_server_connection():
            print("❌ 服务器连接失败，测试终止")
            return
        
        # 获取文件列表
        files = self.get_file_list()
        if not files:
            print("❌ 没有可测试的文件")
            return
        
        # 测试每个文件的下载
        print(f"\n�� 开始测试 {len(files)} 个文件的下载功能")
        for file_info in files:
            filename = file_info['filename']
            download_url = file_info['download_url']
            self.test_download_file(filename, download_url)
        
        # 测试特殊字符文件名
        self.test_special_characters()
        
        # 输出测试总结
        self.print_test_summary()
    
    def print_test_summary(self):
        """打印测试总结"""
        print("\n" + "=" * 50)
        print("📊 测试总结")
        print("=" * 50)
        
        total_tests = len(self.test_results)
        successful_downloads = sum(1 for r in self.test_results if r['status'] == 'success')
        failed_downloads = total_tests - successful_downloads
        
        print(f"总测试数: {total_tests}")
        print(f"成功下载: {successful_downloads}")
        print(f"失败下载: {failed_downloads}")
        print(f"成功率: {(successful_downloads/total_tests*100):.1f}%" if total_tests > 0 else "0%")
        
        if successful_downloads > 0:
            successful_results = [r for r in self.test_results if r['status'] == 'success']
            avg_speed = sum(r['download_speed'] for r in successful_results) / len(successful_results)
            avg_time = sum(r['download_time'] for r in successful_results) / len(successful_results)
            
            print(f"\n📈 性能统计:")
            print(f"平均下载速度: {avg_speed:.0f} 字节/秒")
            print(f"平均下载时间: {avg_time:.2f} 秒")
        
        if failed_downloads > 0:
            print(f"\n❌ 失败详情:")
            for result in self.test_results:
                if result['status'] != 'success':
                    print(f"  - {result['filename']}: {result['status']}")

    def sanitize_filename(self, filename):
        """清理文件名，确保在Windows上有效"""
        # Windows不允许的字符
        invalid_chars = '<>:"/\\|?*'
        
        # 替换无效字符
        for char in invalid_chars:
            filename = filename.replace(char, '_')
        
        # 移除首尾的空格和点
        filename = filename.strip(' .')
        
        # 如果文件名为空，使用默认名称
        if not filename:
            filename = "unnamed_file"
        
        # 限制文件名长度
        if len(filename) > 200:
            filename = filename[:200]
        
        return filename

def main():
    """主函数"""
    print("文件下载测试工具")
    print("=" * 50)
    
    # 可以修改服务器地址
    server_url = input("请输入服务器地址 (默认: http://localhost:8080): ").strip()
    if not server_url:
        server_url = "http://localhost:8080"
    
    tester = DownloadTester(server_url)
    
    try:
        tester.run_all_tests()
    except KeyboardInterrupt:
        print("\n\n⏹️  测试被用户中断")
    except Exception as e:
        print(f"\n\n❌ 测试过程中发生错误: {e}")
    
    input("\n按回车键退出...")

if __name__ == "__main__":
    main() 