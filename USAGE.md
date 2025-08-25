# 使用说明

## 快速开始

### 1. 编译项目
```bash
chmod +x build.sh
./build.sh
```

### 2. 修改测试文件路径
编辑 `test_hardcoded.cpp` 文件，找到这一行：
```cpp
std::string test_file_path = "D:\\Documents\\Drawing1.dwg";
```

将其修改为您的实际文件路径，例如：
```cpp
// Windows 路径示例
std::string test_file_path = "C:\\Users\\YourName\\Documents\\test.pdf";
std::string test_file_path = "D:\\Projects\\drawing.dwg";

// Linux/macOS 路径示例
std::string test_file_path = "/home/username/documents/sample.pdf";
std::string test_file_path = "/Users/username/Desktop/test.dwg";
```

### 3. 运行测试
```bash
cd build
./TestUpload
```

## 测试程序功能

`TestUpload` 程序会自动执行以下操作：

1. **文件检查**: 验证指定路径的文件是否存在
2. **文件读取**: 读取文件内容到内存
3. **文件上传**: 将文件保存到服务器的 `uploads/` 目录
4. **文件验证**: 检查上传后的文件大小和内容
5. **结果展示**: 显示详细的测试结果和文件列表

## 输出示例

```
=== 硬编码文件上传测试 ===
测试文件: D:\Documents\Drawing1.dwg
文件大小: 2048576 字节
文件名: Drawing1.dwg

开始上传文件...
✓ 文件上传成功
✓ 文件验证成功
✓ 文件大小一致
✓ 文件内容验证成功（前100字节）

当前服务器文件列表:
  - Drawing1.dwg (2048576 字节, 2024-01-15 14:30:25)

测试完成！
```

## 常见问题

### Q: 文件路径不存在怎么办？
A: 请检查文件路径是否正确，确保文件确实存在于指定位置。

### Q: 如何测试不同类型的文件？
A: 修改 `test_file_path` 变量，指向您要测试的文件即可。支持所有类型的文件。

### Q: 测试失败怎么办？
A: 查看错误信息，通常是因为：
- 文件路径错误
- 文件权限不足
- 磁盘空间不足

### Q: 可以测试多个文件吗？
A: 可以，但需要修改代码。当前版本每次只能测试一个文件。

## 高级用法

### 批量测试多个文件
如果您需要测试多个文件，可以修改 `test_hardcoded.cpp`：

```cpp
std::vector<std::string> test_files = {
    "D:\\Documents\\Drawing1.dwg",
    "D:\\Documents\\Document1.pdf",
    "D:\\Documents\\Image1.jpg"
};

for (const auto& file_path : test_files) {
    // 测试每个文件
    test_single_file(file_path);
}
```

### 自定义验证逻辑
您可以修改验证逻辑，例如：
- 只验证文件大小
- 验证文件的MD5哈希值
- 验证文件的最后修改时间

## 服务器模式

如果您想启动HTTP服务器进行网络测试：

```bash
cd build
./FileServer 8080
```

然后在另一个终端中：
```bash
# 查看文件列表
curl http://localhost:8080/files

# 下载文件
curl -O http://localhost:8080/download/Drawing1.dwg
```

## 注意事项

1. **文件路径**: 使用正确的路径分隔符（Windows用`\`，Linux/macOS用`/`）
2. **文件权限**: 确保程序有读取指定文件的权限
3. **磁盘空间**: 确保有足够的空间存储上传的文件
4. **文件大小**: 大文件可能需要更多内存和处理时间

## 技术支持

如果遇到问题，请检查：
1. 编译是否成功
2. 文件路径是否正确
3. 文件是否存在且可读
4. 系统权限是否足够 