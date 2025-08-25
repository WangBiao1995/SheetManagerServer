# C++ 文件服务器

一个基于C++的简易高效HTTP文件服务器，支持PDF、DWG等二进制文件的上传、下载和管理。

## 功能特性

- **文件上传**: 支持POST请求上传文件
- **文件下载**: 支持GET请求下载文件
- **文件列表**: 显示所有已上传的文件（JSON格式）
- **文件删除**: 支持DELETE请求删除文件
- **二进制文件支持**: 完全支持PDF、DWG等二进制文件
- **多线程处理**: 每个客户端连接使用独立线程处理
- **安全防护**: 文件名验证和清理，防止路径遍历攻击
- **硬编码测试**: 支持直接指定文件路径进行测试
- **跨平台支持**: 支持Windows、Linux、macOS

## 系统要求

- C++17 或更高版本
- CMake 3.10 或更高版本
- 支持POSIX的系统（Linux、macOS）或Windows

### Windows要求
- Visual Studio 2019 或更高版本（包含C++开发工具）
- 或者 Visual Studio Build Tools 2019+

### Linux/macOS要求
- GCC 7+ 或 Clang 5+
- pthread库

## 编译方法

### Windows系统

#### 方法1: 使用批处理脚本（推荐）
```cmd
build_windows.bat
```

#### 方法2: 手动构建
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### Linux/macOS系统

#### 方法1: 使用Shell脚本
```bash
chmod +x build.sh
./build.sh
```

#### 方法2: 手动构建
```bash
mkdir build
cd build
cmake ..
make
```

编译完成后会生成两个可执行文件：
- `FileServer` / `FileServer.exe` - 主服务器程序
- `TestUpload` / `TestUpload.exe` - 硬编码文件测试程序

## 使用方法

### 方法1: 使用硬编码文件测试（推荐）

1. **修改测试文件路径**
   在 `test_hardcoded.cpp` 中修改文件路径：
   ```cpp
   // Windows路径示例
   std::string test_file_path = "D:\\Documents\\Drawing1.dwg";
   
   // Linux/macOS路径示例
   std::string test_file_path = "/home/username/documents/sample.dwg";
   ```

2. **运行测试程序**
   ```bash
   # Windows
   cd build\Release
   TestUpload.exe
   
   # Linux/macOS
   cd build
   ./TestUpload
   ```

3. **查看结果**
   程序会自动读取指定文件并上传到服务器，显示详细的验证结果。

### 方法2: 启动HTTP服务器

1. **启动服务器**
   ```bash
   # Windows
   cd build\Release
   FileServer.exe        # 使用默认端口8080
   FileServer.exe 9000   # 使用自定义端口9000
   
   # Linux/macOS
   cd build
   ./FileServer        # 使用默认端口8080
   ./FileServer 9000   # 使用自定义端口9000
   ```

2. **测试API接口**
   ```bash
   # 查看文件列表
   curl http://localhost:8080/files
   
   # 下载文件
   curl -O http://localhost:8080/download/Drawing1.dwg
   
   # 删除文件
   curl -X DELETE http://localhost:8080/delete/Drawing1.dwg
   ```

## API接口

### 1. 文件上传
- **方法**: POST
- **路径**: `/upload`
- **内容类型**: `multipart/form-data`
- **参数**: `file` (文件字段)

### 2. 文件下载
- **方法**: GET
- **路径**: `/download/{filename}`
- **响应**: 文件内容，自动设置正确的MIME类型

### 3. 文件列表
- **方法**: GET
- **路径**: `/files`
- **响应**: JSON格式的文件列表

### 4. 文件删除
- **方法**: DELETE
- **路径**: `/delete/{filename}`
- **响应**: 操作结果

## 硬编码测试说明

硬编码测试程序 `TestUpload` 的主要功能：

1. **自动读取指定文件**: 从硬编码路径读取文件
2. **文件验证**: 检查文件大小和内容一致性
3. **详细日志**: 显示每个步骤的执行结果
4. **错误处理**: 友好的错误提示和解决建议

### 修改测试文件路径

在 `test_hardcoded.cpp` 中找到这一行：
```cpp
std::string test_file_path = "D:\\Documents\\Drawing1.dwg";
```

将其修改为您的实际文件路径，例如：
```cpp
// Windows路径示例
std::string test_file_path = "C:\\Users\\YourName\\Documents\\test.pdf";
std::string test_file_path = "D:\\Projects\\drawing.dwg";

// Linux/macOS路径示例
std::string test_file_path = "/home/username/documents/sample.pdf";
std::string test_file_path = "/Users/username/Desktop/test.dwg";
```

## 文件存储

- 上传的文件存储在 `uploads/` 目录中
- 服务器启动时会自动创建该目录
- 文件名会自动清理，移除危险字符

## 安全特性

- 文件名验证和清理
- 防止路径遍历攻击
- 支持的文件类型限制
- 文件大小监控

## 性能优化

- 多线程并发处理
- 二进制文件高效传输
- 内存优化的文件读写
- 非阻塞I/O操作

## 故障排除

### 编译错误

#### Windows系统
- 确保安装了Visual Studio 2019+或Build Tools
- 检查CMake版本是否满足要求
- 使用正确的CMake生成器（如"Visual Studio 16 2019"）

#### Linux/macOS系统
- 确保使用C++17或更高版本
- 检查CMake版本是否满足要求
- 安装必要的开发工具包

### 运行错误
- 检查端口是否被占用
- 确保有足够的文件系统权限
- 检查防火墙设置

### 文件上传失败
- 检查文件大小是否过大
- 确认文件名不包含特殊字符
- 验证Content-Type设置正确

### 硬编码测试失败
- 确保指定的文件路径存在
- 检查文件是否可读
- 验证文件路径格式正确

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。 