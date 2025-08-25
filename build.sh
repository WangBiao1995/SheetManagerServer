#!/bin/bash

# C++文件服务器构建脚本

echo "开始构建C++文件服务器..."

# 检查CMake是否安装
if ! command -v cmake &> /dev/null; then
    echo "错误: 未找到CMake，请先安装CMake"
    exit 1
fi

# 检查编译器是否安装
if ! command -v g++ &> /dev/null; then
    echo "错误: 未找到g++编译器，请先安装g++"
    exit 1
fi

# 创建构建目录
echo "创建构建目录..."
mkdir -p build
cd build

# 配置项目
echo "配置项目..."
cmake ..

# 编译项目
echo "编译项目..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "构建成功！"
    echo "可执行文件位置: build/FileServer"
    echo ""
    echo "运行方法:"
    echo "  cd build"
    echo "  ./FileServer"
    echo ""
    echo "或者指定端口:"
    echo "  ./FileServer 9000"
else
    echo "构建失败！"
    exit 1
fi 