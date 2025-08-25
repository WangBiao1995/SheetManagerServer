cmake_minimum_required(VERSION 3.10)
project(FileServer)

# 保持C++17标准，兼容现有环境
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项 - 根据平台使用不同的编译器选项
if(MSVC)
    # Windows MSVC编译器 - 高性能优化
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /O2 /DNDEBUG /DWIN32_LEAN_AND_MEAN")
    # 启用链接时优化
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
    # GCC/Clang编译器 - 高性能优化
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -DNDEBUG -march=native")
    # 启用链接时优化
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/http_handler.cpp
    src/file_manager.cpp
)

# 头文件
set(HEADERS
    include/server.h
    include/http_handler.h
    include/file_manager.h
)

# 创建主服务器可执行文件
add_executable(FileServer ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(FileServer PRIVATE include)

# 链接库 - Windows下链接ws2_32和mswsock
if(WIN32)
    # Windows系统
    target_link_libraries(FileServer ws2_32 mswsock)
    # 添加Windows Socket库的pragma指令
    target_compile_options(FileServer PRIVATE /DWIN32_LEAN_AND_MEAN)
else()
    # Linux/macOS系统
    target_link_libraries(FileServer pthread)
    # 添加epoll支持
    target_compile_options(FileServer PRIVATE -D_GNU_SOURCE)
endif()

# 创建测试程序
add_executable(TestUpload test_hardcoded.cpp ${HEADERS})
target_include_directories(TestUpload PRIVATE include)

# 链接库 - Windows下链接ws2_32
if(WIN32)
    # Windows系统
    target_link_libraries(TestUpload ws2_32)
    # 添加Windows Socket库的pragma指令
    target_compile_options(TestUpload PRIVATE /DWIN32_LEAN_AND_MEAN)
else()
    # Linux/macOS系统
    target_link_libraries(TestUpload pthread)
endif()

# 安装规则
install(TARGETS FileServer TestUpload DESTINATION bin)
install(DIRECTORY include/ DESTINATION include) 