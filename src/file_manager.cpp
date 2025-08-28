#include "../include/file_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <sys/stat.h>
#endif

FileManager::FileManager(const std::string& upload_dir) 
    : upload_dir_(upload_dir), upload_path_(upload_dir) {
    ensure_upload_directory();
}

FileManager::~FileManager() {}

void FileManager::ensure_upload_directory() {
    if (!std::filesystem::exists(upload_path_)) {
        if (std::filesystem::create_directories(upload_path_)) {
            std::cout << "创建上传目录: " << upload_path_.string() << std::endl;
        } else {
            std::cerr << "创建上传目录失败: " << upload_path_.string() << std::endl;
        }
    }
}

bool FileManager::save_file(const std::string& filename, const std::string& content) {
    if (!is_valid_filename(filename)) {
        std::cerr << "无效的文件名: " << filename << std::endl;
        return false;
    }
    
    std::string sanitized_name = sanitize_filename(filename);
    std::filesystem::path file_path = upload_path_ / sanitized_name;
    
    try {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法创建文件: " << file_path.string() << std::endl;
            return false;
        }
        
        file.write(content.c_str(), content.length());
        file.close();
        
        std::cout << "文件保存成功: " << sanitized_name << " (大小: " << content.length() << " 字节)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存文件时发生异常: " << e.what() << std::endl;
        return false;
    }
}






bool FileManager::save_file(const std::string& filename, const std::vector<char>& content) {
    if (!is_valid_filename(filename)) {
        std::cerr << "无效的文件名: " << filename << std::endl;
        return false;
    }
    
    std::string sanitized_name = sanitize_filename(filename);
    std::filesystem::path file_path = upload_path_ / sanitized_name;
    
    try {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法创建文件: " << file_path.string() << std::endl;
            return false;
        }
        
        file.write(content.data(), content.size());
        file.close();
        
        std::cout << "文件保存成功: " << sanitized_name << " (大小: " << content.size() << " 字节)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存文件时发生异常: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::save_file(const std::string& filename, const char* data, size_t size) {
    if (!is_valid_filename(filename)) {
        std::cerr << "无效的文件名: " << filename << std::endl;
        return false;
    }
    
    std::string sanitized_name = sanitize_filename(filename);
    std::filesystem::path file_path = upload_path_ / sanitized_name;
    
    try {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法创建文件: " << file_path.string() << std::endl;
            return false;
        }
        
        file.write(data, size);
        file.close();
        
        std::cout << "文件保存成功: " << sanitized_name << " (大小: " << size << " 字节)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存文件时发生异常: " << e.what() << std::endl;
        return false;
    }
}

std::vector<char> FileManager::read_file(const std::string& filename) {
    std::vector<char> content;
    
    if (!is_valid_filename(filename)) {
        std::cerr << "无效的文件名: " << filename << std::endl;
        return content;
    }
    
    std::string sanitized_name = sanitize_filename(filename);
    std::filesystem::path file_path = upload_path_ / sanitized_name;
    
    if (!std::filesystem::exists(file_path)) {
        std::cerr << "文件不存在: " << file_path.string() << std::endl;
        return content;
    }
    
    try {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << file_path.string() << std::endl;
            return content;
        }
        
        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        content.resize(file_size);
        if (!file.read(content.data(), file_size)) {
            std::cerr << "读取文件失败: " << file_path.string() << std::endl;
            content.clear();
        }
        
        file.close();
        std::cout << "文件读取成功: " << sanitized_name << " (大小: " << content.size() << " 字节)" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "读取文件时发生异常: " << e.what() << std::endl;
        content.clear();
    }
    
    return content;
}

bool FileManager::file_exists(const std::string& filename) {
    // 首先检查文件名是否有效
    if (!is_valid_filename(filename)) {
        std::cerr << "文件名无效，无法检查文件存在性: " << filename << std::endl;
        return false;
    }
    
    try {
        // 使用原始文件名构建路径
        std::filesystem::path file_path = upload_path_ / filename;
        
        // 检查路径是否有效
        if (file_path.empty()) {
            std::cerr << "构建的文件路径为空" << std::endl;
            return false;
        }
        
        // 检查文件是否存在
        bool exists = std::filesystem::exists(file_path);
        
        std::cout << "文件存在性检查: " << filename << " -> " << (exists ? "存在" : "不存在") << std::endl;
        
        return exists;
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "文件系统错误: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "检查文件存在性时发生异常: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "检查文件存在性时发生未知异常" << std::endl;
        return false;
    }
}

bool FileManager::file_exists(const std::wstring& filename) {
#ifdef _WIN32
    // 在Windows上使用宽字符API检查文件是否存在
    std::wstring full_path = upload_path_.wstring() + L"\\" + filename;
    
    std::cout << "宽字符文件存在检查: " << std::endl;
    std::cout << "  原始文件名: " << wstring_to_utf8(filename) << std::endl;
    std::cout << "  完整路径: " << wstring_to_utf8(full_path) << std::endl;
    
    // 使用_wstat检查文件是否存在
    struct _stat64i32 buffer;
    int result = _wstat(full_path.c_str(), &buffer);
    
    bool exists = (result == 0);
    std::cout << "  文件存在: " << (exists ? "是" : "否") << std::endl;
    
    return exists;
#else
    // 非Windows系统，转换为string后调用原函数
    std::string str_filename = wstring_to_utf8(filename);
    return file_exists(str_filename);
#endif
}

bool FileManager::delete_file(const std::string& filename) {
    if (!is_valid_filename(filename)) {
        std::cerr << "无效的文件名: " << filename << std::endl;
        return false;
    }
    
    std::string sanitized_name = sanitize_filename(filename);
    std::filesystem::path file_path = upload_path_ / sanitized_name;
    
    if (!std::filesystem::exists(file_path)) {
        std::cerr << "文件不存在: " << file_path.string() << std::endl;
        return false;
    }
    
    try {
        if (std::filesystem::remove(file_path)) {
            std::cout << "文件删除成功: " << sanitized_name << std::endl;
            return true;
        } else {
            std::cerr << "文件删除失败: " << file_path.string() << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "删除文件时发生异常: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::delete_file(const std::wstring& filename) {
#ifdef _WIN32
    std::wstring full_path = upload_path_.wstring() + L"\\" + filename;
    
    std::cout << "宽字符文件删除: " << std::endl;
    std::cout << "  文件名: " << wstring_to_utf8(filename) << std::endl;
    std::cout << "  完整路径: " << wstring_to_utf8(full_path) << std::endl;
    
    // 先检查文件是否存在
    if (!file_exists(filename)) {
        std::cerr << "文件不存在: " << wstring_to_utf8(full_path) << std::endl;
        return false;
    }
    
    try {
        // 使用_wremove删除文件
        int result = _wremove(full_path.c_str());
        if (result == 0) {
            std::cout << "文件删除成功: " << wstring_to_utf8(filename) << std::endl;
            return true;
        } else {
            std::cerr << "文件删除失败: " << wstring_to_utf8(full_path) << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "删除文件时发生异常: " << e.what() << std::endl;
        return false;
    }
#else
    // 非Windows系统，转换为string后调用原函数
    std::string str_filename = wstring_to_utf8(filename);
    return delete_file(str_filename);
#endif
}

// UTF-8到UTF-16转换函数
std::wstring FileManager::utf8_to_wstring(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return L"";
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                                          (int)str.size(), NULL, 0);
    if (size_needed == 0) {
        std::cerr << "UTF-8到UTF-16转换失败，错误码: " << GetLastError() << std::endl;
        return L"";
    }
    
    std::wstring wstr(size_needed, 0);
    int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                                    (int)str.size(), &wstr[0], size_needed);
    
    if (result == 0) {
        std::cerr << "UTF-8到UTF-16转换失败，错误码: " << GetLastError() << std::endl;
        return L"";
    }
    
    std::cout << "UTF-8到UTF-16转换成功: '" << str << "' -> '" << wstring_to_utf8(wstr) << "'" << std::endl;
    return wstr;
#else
    // 非Windows系统，直接返回转换后的字符串
    std::wstring wstr(str.begin(), str.end());
    return wstr;
#endif
}

// UTF-16到UTF-8转换函数
std::string FileManager::wstring_to_utf8(const std::wstring& wstr) {
#ifdef _WIN32
    if (wstr.empty()) return "";
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                                          (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed == 0) {
        std::cerr << "UTF-16到UTF-8转换失败，错误码: " << GetLastError() << std::endl;
        return "";
    }
    
    std::string str(size_needed, 0);
    int result = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(),
                                    (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    
    if (result == 0) {
        std::cerr << "UTF-16到UTF-8转换失败，错误码: " << GetLastError() << std::endl;
        return "";
    }
    
    return str;
#else
    // 非Windows系统，直接返回转换后的字符串
    std::string str(wstr.begin(), wstr.end());
    return str;
#endif
}

std::vector<FileInfo> FileManager::list_files() {
    std::vector<FileInfo> files;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(upload_path_)) {
            if (entry.is_regular_file()) {
                FileInfo file_info;
                
                // 获取文件名，确保编码一致性
                std::string filename = entry.path().filename().string();
                
                // 输出详细的调试信息
                std::cout << "发现文件: " << filename << std::endl;
                std::cout << "文件名长度: " << filename.length() << std::endl;
                
                // 检查文件名编码 - 输出每个字节的十六进制值
                std::cout << "文件名字节 (hex): ";
                for (unsigned char c : filename) {
                    printf("%02X ", c);
                }
                std::cout << std::endl;
                
                file_info.filename = filename;
                file_info.path = entry.path().string();
                file_info.size = std::filesystem::file_size(entry.path());
                
                auto time_point = std::filesystem::last_write_time(entry.path());
                // C++17兼容的时间转换方法
                auto duration = time_point.time_since_epoch();
                auto system_duration = std::chrono::duration_cast<std::chrono::system_clock::duration>(duration);
                auto system_time = std::chrono::system_clock::time_point(system_duration);
                auto time_t = std::chrono::system_clock::to_time_t(system_time);
                
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                file_info.last_modified = ss.str();
                
                // 根据文件扩展名设置MIME类型
                std::string ext = filename.substr(filename.find_last_of('.') + 1);
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == "pdf") file_info.mime_type = "application/pdf";
                else if (ext == "dwg") file_info.mime_type = "application/acad";
                else if (ext == "txt") file_info.mime_type = "text/plain";
                else if (ext == "html" || ext == "htm") file_info.mime_type = "text/html";
                else if (ext == "css") file_info.mime_type = "text/css";
                else if (ext == "js") file_info.mime_type = "application/javascript";
                else if (ext == "json") file_info.mime_type = "application/json";
                else if (ext == "xml") file_info.mime_type = "application/xml";
                else if (ext == "jpg" || ext == "jpeg") file_info.mime_type = "image/jpeg";
                else if (ext == "png") file_info.mime_type = "image/png";
                else if (ext == "gif") file_info.mime_type = "image/gif";
                else if (ext == "bmp") file_info.mime_type = "image/bmp";
                else if (ext == "ico") file_info.mime_type = "image/x-icon";
                else file_info.mime_type = "application/octet-stream";
                
                files.push_back(file_info);
            }
        }
        
        // 按文件名排序
        std::sort(files.begin(), files.end(), 
                  [](const FileInfo& a, const FileInfo& b) {
                      return a.filename < b.filename;
                  });
    } catch (const std::exception& e) {
        std::cerr << "列出文件时发生异常: " << e.what() << std::endl;
    }
    
    return files;
}

std::string FileManager::get_file_path(const std::string& filename) {
    if (!is_valid_filename(filename)) {
        return "";
    }
    
    std::string sanitized_name = sanitize_filename(filename);
    std::filesystem::path file_path = upload_path_ / sanitized_name;
    return file_path.string();
}

bool FileManager::is_valid_filename(const std::string& filename) {
    if (filename.empty() || filename.length() > 255) {
        return false;
    }
    
    // 检查是否包含危险字符
    std::string dangerous_chars = "<>:\"/\\|?*";
    for (char c : dangerous_chars) {
        if (filename.find(c) != std::string::npos) {
            return false;
        }
    }
    
    // 检查是否以点开头或结尾
    if (filename.front() == '.' || filename.back() == '.') {
        return false;
    }
    
    // 检查是否包含连续的点
    if (filename.find("..") != std::string::npos) {
        return false;
    }
    
    return true;
}

std::string FileManager::sanitize_filename(const std::string& filename) {
    std::string sanitized = filename;
    
    // 替换危险字符为下划线
    std::string dangerous_chars = "<>:\"/\\|?*";
    for (char c : dangerous_chars) {
        std::replace(sanitized.begin(), sanitized.end(), c, '_');
    }
    
    // 去除首尾的点
    while (!sanitized.empty() && sanitized.front() == '.') {
        sanitized.erase(0, 1);
    }
    while (!sanitized.empty() && sanitized.back() == '.') {
        sanitized.pop_back();
    }
    
    // 如果文件名为空，使用默认名称
    if (sanitized.empty()) {
        sanitized = "unnamed_file";
    }
    
    return sanitized;
}

std::string FileManager::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
} 