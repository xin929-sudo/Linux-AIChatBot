# CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(AIChatServer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译类型（Debug/Release）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 设置头文件目录
include_directories(${CMAKE_SOURCE_DIR}/../inc)

# 查找所有源文件
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/../src/*.cpp" "${CMAKE_SOURCE_DIR}/../src/*.c")

# 查找所有头文件（可选，用于IDE显示）
file(GLOB_RECURSE HEADERS "${CMAKE_SOURCE_DIR}/../inc/*.h" "${CMAKE_SOURCE_DIR}/../inc/*.hpp")
# 添加 MySQL 头文件路径
include_directories(/usr/include/mysql-cppconn-8)
# 打印找到的文件（调试用）
message(STATUS "Found source files: ${SOURCES}")
message(STATUS "Found header files: ${HEADERS}")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接pthread库（线程池需要）
find_package(Threads REQUIRED)
# 查找curl库
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES}
                Threads::Threads                        
                mysqlcppconn)

# 如果需要链接其他库，可以添加
# target_link_libraries(${PROJECT_NAME} other_library)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 安装规则（可选）
# install(TARGETS ${PROJECT_NAME}
#     RUNTIME DESTINATION bin
# )

# 如果要生成静态库或动态库，可以使用以下代码
# add_library(${PROJECT_NAME}_lib STATIC ${SOURCES})
# add_library(${PROJECT_NAME}_shared SHARED ${SOURCES})

# 添加编译定义（可选）
# target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE=1)