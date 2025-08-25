#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

struct FileInfo {
    std::string filename;
    std::string path;
    size_t size;
    std::string last_modified;
    std::string mime_type;
};

class FileManager {
public:
    FileManager(const std::string& upload_dir = "uploads");
    ~FileManager();
    
    bool save_file(const std::string& filename, const std::string& content);
    bool save_file(const std::string& filename, const std::vector<char>& content);
    bool save_file(const std::string& filename, const char* data, size_t size);
    
    std::vector<char> read_file(const std::string& filename);
    
    // 修改为支持宽字符的文件操作函数
    bool file_exists(const std::string& filename);
    bool file_exists(const std::wstring& filename);
    bool delete_file(const std::string& filename);
    bool delete_file(const std::wstring& filename);
    
    std::vector<FileInfo> list_files();
    std::string get_file_path(const std::string& filename);
    
    static bool is_valid_filename(const std::string& filename);
    static std::string sanitize_filename(const std::string& filename);
    
    // 添加UTF-8到UTF-16转换函数
    static std::wstring utf8_to_wstring(const std::string& str);
    static std::string wstring_to_utf8(const std::wstring& wstr);
    
private:
    std::string upload_dir_;
    std::filesystem::path upload_path_;
    
    void ensure_upload_directory();
    std::string get_current_timestamp();
};

#endif // FILE_MANAGER_H 