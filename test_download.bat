@echo off
chcp 65001 >nul
title 文件下载测试工具

echo ========================================
echo           文件下载测试工具
echo ========================================
echo.

:: 检查Python是否安装
python --version >nul 2>&1
if errorlevel 1 (
    echo ❌ 错误: 未找到Python，请先安装Python 3.7+
    echo 下载地址: https://www.python.org/downloads/
    pause
    exit /b 1
)

:: 检查requests库是否安装
python -c "import requests" >nul 2>&1
if errorlevel 1 (
    echo 📦 正在安装requests库...
    pip install requests
    if errorlevel 1 (
        echo ❌ 安装requests库失败
        pause
        exit /b 1
    )
    echo ✅ requests库安装成功
)

echo ✅ 环境检查完成
echo.

:: 运行测试脚本
echo 🚀 启动下载测试...
python test_download.py

echo.
echo 测试完成！
pause 