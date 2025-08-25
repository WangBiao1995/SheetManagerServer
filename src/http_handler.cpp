#include "../include/http_handler.h"
#include "../include/file_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>

HttpHandler::HttpHandler() {}

HttpRequest HttpHandler::parse_request(const std::string& raw_request) {
    HttpRequest request;
    
    std::cout << "原始请求数据长度: " << raw_request.length() << std::endl;
    std::cout << "原始请求前200字符: " << raw_request.substr(0, 200) << std::endl;
    
    // 查找请求行和头部的分界线
    size_t header_end = raw_request.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        header_end = raw_request.find("\n\n");
    }
    
    std::cout << "头部结束位置: " << header_end << std::endl;
    
    std::string headers_text = raw_request.substr(0, header_end);
    if (header_end != std::string::npos && header_end + 4 < raw_request.length()) {
        request.body = raw_request.substr(header_end + 4);
    }
    
    // 解析请求行
    std::istringstream header_stream(headers_text);
    std::string line;
    if (std::getline(header_stream, line)) {
        std::istringstream line_stream(line);
        line_stream >> request.method >> request.path >> request.version;
        std::cout << "解析结果 - 方法: '" << request.method << "', 路径: '" << request.path << "', 版本: '" << request.version << "'" << std::endl;
    }
    
    // 解析头部
    while (std::getline(header_stream, line) && !line.empty() && line != "\r") {
        if (line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        
        auto header_pair = parse_header_line(line);
        if (!header_pair.first.empty()) {
            request.headers[header_pair.first] = header_pair.second;
            std::cout << "请求头: " << header_pair.first << ": " << header_pair.second << std::endl;
        }
    }
    
    return request;
}

std::string HttpHandler::build_response(const HttpResponse& response) {
    std::ostringstream oss;
    
    // 状态行
    oss << "HTTP/1.1 " << response.status_code << " " << response.status_text << "\r\n";
    
    // 添加必要的HTTP头
    oss << "Content-Length: " << response.body.length() << "\r\n";
    oss << "Connection: close\r\n";
    
    // 安全地获取Content-Type，如果不存在则使用默认值
    auto content_type_it = response.headers.find("Content-Type");
    std::string content_type = (content_type_it != response.headers.end()) ? 
                               content_type_it->second : "text/plain; charset=utf-8";
    
    // 确保Content-Type包含字符集信息
    if (content_type.find("charset=") == std::string::npos) {
        content_type += "; charset=utf-8";
    }
    
    oss << "Content-Type: " << content_type << "\r\n";
    
    // 添加所有其他自定义HTTP头
    for (const auto& header : response.headers) {
        if (header.first != "Content-Type" && header.first != "Content-Length") {
            oss << header.first << ": " << header.second << "\r\n";
        }
    }
    
    // 空行
    oss << "\r\n";
    
    // 响应体
    oss << response.body;
    
    return oss.str();
}

HttpResponse HttpHandler::handle_upload(const HttpRequest& request) {
    HttpResponse response;
    
    // 检查Content-Type是否为multipart/form-data
    auto content_type_it = request.headers.find("Content-Type");
    if (content_type_it == request.headers.end() || 
        content_type_it->second.find("multipart/form-data") == std::string::npos) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "Content-Type必须是multipart/form-data";
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    
    // 解析multipart数据
    std::string boundary;
    size_t boundary_pos = content_type_it->second.find("boundary=");
    if (boundary_pos != std::string::npos) {
        boundary = content_type_it->second.substr(boundary_pos + 9);
        if (boundary.front() == '"') boundary = boundary.substr(1);
        if (boundary.back() == '"') boundary.pop_back();
    }
    
    if (boundary.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "无法解析boundary";
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    
    // 查找文件数据
    std::string file_data;
    std::string filename;
    
    size_t pos = request.body.find("--" + boundary);
    while (pos != std::string::npos) {
        size_t next_boundary = request.body.find("--" + boundary, pos + boundary.length() + 2);
        if (next_boundary == std::string::npos) break;
        
        std::string part = request.body.substr(pos, next_boundary - pos);
        
        // 查找文件名
        size_t filename_pos = part.find("filename=\"");
        if (filename_pos != std::string::npos) {
            size_t filename_start = filename_pos + 10;
            size_t filename_end = part.find("\"", filename_start);
            if (filename_end != std::string::npos) {
                filename = part.substr(filename_start, filename_end - filename_start);
            }
        }
        
        // 查找文件数据开始位置
        size_t data_start = part.find("\r\n\r\n");
        if (data_start != std::string::npos) {
            file_data = part.substr(data_start + 4);
            break;
        }
        
        pos = next_boundary;
    }
    
    if (filename.empty() || file_data.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "未找到文件数据";
        response.headers["Content-Type"] = "text/plain";
        return response;
    }
    
    // 保存文件
    FileManager file_manager;
    if (file_manager.save_file(filename, file_data)) {
        response.status_code = 200;
        response.status_text = "OK";
        response.body = "文件上传成功: " + filename;
        response.headers["Content-Type"] = "text/plain";
    } else {
        response.status_code = 500;
        response.status_text = "Internal Server Error";
        response.body = "文件保存失败";
        response.headers["Content-Type"] = "text/plain";
    }
    
    return response;
}

HttpResponse HttpHandler::handle_download(const HttpRequest& request) {
    HttpResponse response;
    
    // 检查请求路径是否有效
    if (request.path.length() < 11) {  // "/download/" 长度为10
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "无效的下载路径";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 提取文件名
    std::string filename = request.path.substr(10); // 去掉"/download/"
    std::string original_filename = filename;
    
    // 检查提取的文件名是否为空
    if (filename.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名不能为空";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // URL解码
    try {
        filename = url_decode(filename);
    } catch (const std::exception& e) {
        std::cerr << "URL解码异常: " << e.what() << std::endl;
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名解码失败";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    std::cout << "下载请求 - 原始路径: " << request.path << std::endl;
    std::cout << "下载请求 - URL编码的文件名: " << original_filename << std::endl;
    std::cout << "下载请求 - 解码后的文件名: " << filename << std::endl;
    std::cout << "下载请求 - 文件名长度: " << filename.length() << std::endl;
    
    // 再次检查解码后的文件名
    if (filename.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "解码后的文件名不能为空";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名是否包含危险字符
    if (filename.find("..") != std::string::npos || 
        filename.find("\\") != std::string::npos || 
        filename.find("/") != std::string::npos) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名包含危险字符";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名是否包含其他无效字符
    std::string invalid_chars = "<>:\"|?*";
    for (char c : invalid_chars) {
        if (filename.find(c) != std::string::npos) {
            response.status_code = 400;
            response.status_text = "Bad Request";
            response.body = "文件名包含无效字符: " + std::string(1, c);
            response.headers["Content-Type"] = "text/plain; charset=utf-8";
            return response;
        }
    }
    
    // 检查文件名是否以点开头或结尾
    if (filename.front() == '.' || filename.back() == '.') {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名不能以点开头或结尾";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名长度
    if (filename.length() > 255) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名过长";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名是否为空（再次确认）
    if (filename.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名不能为空";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    std::cout << "文件名验证通过，开始检查文件是否存在..." << std::endl;
    
    // 检查文件是否存在
    FileManager file_manager;
    try {
        if (!file_manager.file_exists(filename)) {
            response.status_code = 404;
            response.status_text = "Not Found";
            response.body = "文件不存在: " + filename;
            response.headers["Content-Type"] = "text/plain; charset=utf-8";
            return response;
        }
    } catch (const std::exception& e) {
        std::cerr << "检查文件存在性时发生异常: " << e.what() << std::endl;
        response.status_code = 500;
        response.status_text = "Internal Server Error";
        response.body = "文件系统访问异常";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 读取文件
    std::vector<char> file_content = file_manager.read_file(filename);
    if (file_content.empty()) {
        response.status_code = 500;
        response.status_text = "Internal Server Error";
        response.body = "读取文件失败";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 设置响应
    response.status_code = 200;
    response.status_text = "OK";
    response.body = std::string(file_content.begin(), file_content.end());
    
    // 安全地获取MIME类型
    try {
        response.headers["Content-Type"] = get_mime_type(filename);
    } catch (const std::exception& e) {
        std::cerr << "获取MIME类型异常: " << e.what() << std::endl;
        response.headers["Content-Type"] = "application/octet-stream";
    }
    
    response.headers["Content-Length"] = std::to_string(file_content.size());
    
    // 关键修改：对文件名进行URL编码，确保Content-Disposition头正确
    // 同时添加调试信息
    std::string encoded_filename = url_encode(filename);
    std::cout << "下载响应 - 原始文件名: " << filename << std::endl;
    std::cout << "下载响应 - 编码后文件名: " << encoded_filename << std::endl;
    
    response.headers["Content-Disposition"] = "attachment; filename=\"" + encoded_filename + "\"";
    
    return response;
}

HttpResponse HttpHandler::handle_list_files(const HttpRequest& request) {
    HttpResponse response;
    
    FileManager file_manager;
    std::vector<FileInfo> files = file_manager.list_files();
    
    // 生成JSON格式的文件列表，确保使用正确的UTF-8编码
    std::ostringstream json;
    json << "{\n";
    json << "  \"status\": \"success\",\n";
    json << "  \"message\": \"文件列表获取成功\",\n";  // 确保这是正确的UTF-8
    json << "  \"count\": " << files.size() << ",\n";
    json << "  \"files\": [\n";
    
    for (size_t i = 0; i < files.size(); ++i) {
        const auto& file = files[i];
        json << "    {\n";
        
        // 关键：确保文件名正确输出到JSON
        json << "      \"filename\": \"";
        
        // 输出每个字符的调试信息
        std::cout << "JSON输出文件名: " << file.filename << std::endl;
        std::cout << "JSON文件名长度: " << file.filename.length() << std::endl;
        
        // 逐字符输出，确保编码正确
        for (char c : file.filename) {
            json << c;
        }
        
        json << "\",\n";
        json << "      \"size\": " << file.size << ",\n";
        json << "      \"last_modified\": \"" << file.last_modified << "\",\n";
        json << "      \"mime_type\": \"" << file.mime_type << "\",\n";
        
        // 生成下载URL时使用正确的编码
        std::string encoded_filename = url_encode(file.filename);
        json << "      \"download_url\": \"/download/" << encoded_filename << "\",\n";
        json << "      \"delete_url\": \"/delete/" << encoded_filename << "\"\n";
        
        json << "    }";
        if (i < files.size() - 1) json << ",";
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}";
    
    response.status_code = 200;
    response.status_text = "OK";
    response.body = json.str();
    
    // 关键：确保Content-Type包含正确的字符集
    response.headers["Content-Type"] = "application/json; charset=utf-8";
    
    // 添加调试信息
    std::cout << "生成的JSON响应体长度: " << response.body.length() << std::endl;
    std::cout << "JSON响应体前200字符: " << response.body.substr(0, 200) << std::endl;
    
    return response;
}

HttpResponse HttpHandler::handle_delete_file(const HttpRequest& request) {
    HttpResponse response;
    
    // 检查请求路径是否有效
    if (request.path.length() < 9) {  // "/delete/" 长度为8
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "无效的删除路径";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 提取文件名
    std::string filename = request.path.substr(8); // 去掉"/delete/"
    
    // 检查提取的文件名是否为空
    if (filename.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名不能为空";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // URL解码
    try {
        filename = url_decode(filename);
    } catch (const std::exception& e) {
        std::cerr << "URL解码异常: " << e.what() << std::endl;
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名解码失败";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查解码后的文件名
    if (filename.empty()) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "解码后的文件名不能为空";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名是否包含危险字符
    if (filename.find("..") != std::string::npos || 
        filename.find("\\") != std::string::npos || 
        filename.find("/") != std::string::npos) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名包含危险字符";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名是否包含其他无效字符
    std::string invalid_chars = "<>:\"|?*";
    for (char c : invalid_chars) {
        if (filename.find(c) != std::string::npos) {
            response.status_code = 400;
            response.status_text = "Bad Request";
            response.body = "文件名包含无效字符: " + std::string(1, c);
            response.headers["Content-Type"] = "text/plain; charset=utf-8";
            return response;
        }
    }
    
    // 检查文件名是否以点开头或结尾
    if (filename.front() == '.' || filename.back() == '.') {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名不能以点开头或结尾";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    // 检查文件名长度
    if (filename.length() > 255) {
        response.status_code = 400;
        response.status_text = "Bad Request";
        response.body = "文件名过长";
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
        return response;
    }
    
    std::cout << "文件名验证通过，开始删除文件..." << std::endl;
    
    // 删除文件
    FileManager file_manager;
    try {
        if (file_manager.delete_file(filename)) {
            response.status_code = 200;
            response.status_text = "OK";
            response.body = "文件删除成功: " + filename;
            response.headers["Content-Type"] = "text/plain; charset=utf-8";
        } else {
            response.status_code = 500;
            response.status_text = "Internal Server Error";
            response.body = "文件删除失败";
            response.headers["Content-Type"] = "text/plain; charset=utf-8";
        }
    } catch (const std::exception& e) {
        std::cerr << "删除文件时发生异常: " << e.what() << std::endl;
        response.status_code = 500;
        response.status_text = "Internal Server Error";
        response.body = "文件删除异常: " + std::string(e.what());
        response.headers["Content-Type"] = "text/plain; charset=utf-8";
    }
    
    return response;
}

std::string HttpHandler::get_mime_type(const std::string& filename) {
    // 检查文件名是否有效
    if (filename.empty()) {
        return "application/octet-stream";
    }
    
    // 查找文件扩展名
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos == filename.length() - 1) {
        // 没有扩展名或扩展名为空
        return "application/octet-stream";
    }
    
    // 安全地提取扩展名
    std::string ext = filename.substr(dot_pos + 1);
    
    // 检查扩展名是否为空
    if (ext.empty()) {
        return "application/octet-stream";
    }
    
    // 转换为小写
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == "pdf") return "application/pdf";
    if (ext == "dwg") return "application/acad";
    if (ext == "txt") return "text/plain";
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "xml") return "application/xml";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "bmp") return "image/bmp";
    if (ext == "ico") return "image/x-icon";
    
    return "application/octet-stream";
}

std::string HttpHandler::url_decode(const std::string& encoded) {
    std::string result;
    
    // 检查输入是否有效
    if (encoded.empty()) {
        return result;
    }
    
    result.reserve(encoded.length());
    
    try {
        for (size_t i = 0; i < encoded.length(); ++i) {
            if (encoded[i] == '%' && i + 2 < encoded.length()) {
                // 处理URL编码的十六进制字符
                std::string hex_str = encoded.substr(i + 1, 2);
                int decoded_value;
                
                // 验证十六进制字符串的有效性
                if (hex_str.length() == 2 && 
                    ((hex_str[0] >= '0' && hex_str[0] <= '9') || 
                     (hex_str[0] >= 'A' && hex_str[0] <= 'F') || 
                     (hex_str[0] >= 'a' && hex_str[0] <= 'f')) &&
                    ((hex_str[1] >= '0' && hex_str[1] <= '9') || 
                     (hex_str[1] >= 'A' && hex_str[1] <= 'F') || 
                     (hex_str[1] >= 'a' && hex_str[1] <= 'f'))) {
                    
                    // 使用更兼容的十六进制解析方法
                    if (sscanf(hex_str.c_str(), "%2x", &decoded_value) == 1) {
                        // 确保值在有效范围内
                        if (decoded_value >= 0 && decoded_value <= 255) {
                            result += static_cast<char>(decoded_value);
                            i += 2;  // 跳过已处理的字符
                        } else {
                            // 值无效，保持原样并记录错误
                            std::cerr << "警告: 无效的十六进制值: " << hex_str << std::endl;
                            result += encoded[i];
                        }
                    } else {
                        // 解析失败，保持原样并记录错误
                        std::cerr << "警告: 十六进制解析失败: " << hex_str << std::endl;
                        result += encoded[i];
                    }
                } else {
                    // 无效的十六进制字符串，保持原样
                    std::cerr << "警告: 无效的十六进制字符串: " << hex_str << std::endl;
                    result += encoded[i];
                }
            } else if (encoded[i] == '+') {
                result += ' ';
            } else {
                result += encoded[i];
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "URL解码异常: " << e.what() << std::endl;
        // 发生异常时返回原始字符串
        return encoded;
    }
    
    return result;
}

std::string HttpHandler::url_encode(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 3);  // 预留足够的空间
    
    for (unsigned char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            // 安全字符，直接添加
            result += c;
        } else {
            // 需要编码的字符
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            result += hex;
        }
    }
    
    return result;
}

std::vector<std::string> HttpHandler::parse_headers(const std::string& header_text) {
    std::vector<std::string> headers;
    std::istringstream stream(header_text);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            headers.push_back(line);
        }
    }
    
    return headers;
}

std::pair<std::string, std::string> HttpHandler::parse_header_line(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        return {"", ""};
    }
    
    std::string key = line.substr(0, colon_pos);
    std::string value = line.substr(colon_pos + 1);
    
    // 去除首尾空格
    key.erase(0, key.find_first_not_of(" \t"));
    key.erase(key.find_last_not_of(" \t") + 1);
    value.erase(0, value.find_first_not_of(" \t"));
    value.erase(value.find_last_not_of(" \t") + 1);
    
    return {key, value};
} 