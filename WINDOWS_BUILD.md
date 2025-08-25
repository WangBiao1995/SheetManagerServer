# Windows构建说明

## 前置要求

### 1. 安装Visual Studio
- 下载并安装 [Visual Studio Community 2019](https://visualstudio.microsoft.com/downloads/) 或更高版本
- 在安装时选择"使用C++的桌面开发"工作负载
- 或者安装 [Visual Studio Build Tools 2019](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019)

### 2. 安装CMake
- 下载并安装 [CMake](https://cmake.org/download/)
- 确保CMake在系统PATH中

### 3. 验证安装
打开命令提示符，运行：
```cmd
cl
cmake --version
```

如果显示版本信息，说明安装成功。

## 构建步骤

### 方法1: 使用简化批处理脚本（推荐）

1. 双击运行 `build_windows_simple.bat`
2. 等待构建完成
3. 查看输出信息

### 方法2: 使用原始批处理脚本

1. 双击运行 `build_windows.bat`
2. 等待构建完成
3. 查看输出信息

### 方法3: 手动构建

1. **打开开发者命令提示符**
   - 开始菜单 → Visual Studio 2019 → Developer Command Prompt for VS 2019
   - 或者运行 `vcvars64.bat`

2. **创建构建目录**
   ```cmd
   mkdir build
   cd build
   ```

3. **配置项目（选择一种）**
   ```cmd
   # 方法1: 使用MinGW Makefiles（推荐）
   cmake .. -G "MinGW Makefiles"
   
   # 方法2: 使用NMake Makefiles
   cmake .. -G "NMake Makefiles"
   
   # 方法3: 使用Visual Studio生成器
   cmake .. -G "Visual Studio 16 2019" -A x64
   ```

4. **编译项目**
   ```cmd
   cmake --build .
   ```

## 构建输出

构建成功后，可执行文件位于：
- `build\FileServer.exe` - 主服务器程序
- `build\TestUpload.exe` - 测试程序

## 常见问题

### Q: 找不到cl.exe
A: 请使用"Developer Command Prompt for VS 2019"或运行vcvars64.bat

### Q: CMake配置失败
A: 检查CMake版本，确保使用3.10或更高版本

### Q: 编译错误 E0311
A: 这是Windows头文件冲突导致的，已在新版本中修复。请使用最新的代码。

### Q: 链接错误
A: 检查是否包含了ws2_32.lib库，CMakeLists.txt已自动处理

### Q: 生成器不支持
A: 尝试使用不同的CMake生成器：
- `MinGW Makefiles` - 最兼容
- `NMake Makefiles` - 需要Visual Studio
- `Visual Studio 16 2019` - 需要完整Visual Studio

## 故障排除

### 编译错误 E0311
如果遇到 "无法重载仅按返回类型区分的函数" 错误：

1. **原因**: Windows头文件与POSIX函数冲突
2. **解决方案**: 使用最新版本的代码，已修复此问题
3. **预防**: 避免使用 `close` 宏，改用 `socket_close`

### 生成器问题
如果某个CMake生成器不工作：

1. **MinGW Makefiles**: 最兼容，推荐使用
2. **NMake Makefiles**: 需要Visual Studio命令行工具
3. **Visual Studio**: 需要完整Visual Studio安装

### 依赖问题
如果缺少依赖：

1. **ws2_32.lib**: CMakeLists.txt已自动链接
2. **C++17支持**: 确保Visual Studio 2017+
3. **CMake版本**: 确保3.10+

## 运行程序

### 1. 修改测试文件路径
编辑 `test_hardcoded.cpp`，修改文件路径：
```cpp
std::string test_file_path = "C:\\Users\\YourName\\Documents\\test.pdf";
```

### 2. 运行测试程序
```cmd
cd build
TestUpload.exe
```

### 3. 启动服务器
```cmd
cd build
FileServer.exe
```

## 注意事项

1. **路径分隔符**: Windows使用反斜杠 `\`
2. **文件权限**: 确保程序有读取指定文件的权限
3. **防火墙**: 首次运行服务器时可能需要允许防火墙访问
4. **端口占用**: 确保8080端口未被其他程序占用
5. **头文件冲突**: 避免使用可能冲突的宏定义

## 技术支持

如果遇到问题，请检查：
1. Visual Studio安装是否完整
2. CMake版本是否满足要求
3. 是否使用了正确的命令提示符
4. 文件路径是否正确
5. 是否使用了最新版本的代码

## 更新日志

- **v1.1**: 修复Windows编译错误E0311
- **v1.0**: 初始版本，支持跨平台编译 