cmake_minimum_required(VERSION 3.0)
project(will)

# wrapper for add_executable
function(will_add_executable targetname srcs depends libs)
    add_executable(${targetname} ${srcs})
    add_dependencies(${targetname} ${depends})
    target_link_libraries(${targetname} ${libs})
endfunction()

set(CMAKE_VERBOSE_MAKEFILE ON)

# 指定编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -ggdb -Wall -Werror")

# -rdynamic: 将所有符号都加入到符号表中，便于使用dlopen或者backtrace追踪到符号
# -fPIC: 生成位置无关的代码，便于动态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -fPIC")

# -Wno-unused-function: 不要警告未使用函数
# -Wno-builtin-macro-redefined: 不要警告内置宏重定义，用于重定义内置的__FILE__宏
# -Wno-deprecated: 不要警告过时的特性
# -Wno-deprecated-declarations: 不要警告使用带deprecated属性的变量，类型，函数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated -Wno-deprecated-declarations")

include_directories(.)

option(BUILD_TEST "ON for complile test" ON)

find_package(Boost REQUIRED) 
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

set(LIB_SRC
    will/address.cc
    will/bytearray.cc
    will/fd_manager.cc 
    will/fiber.cc
    will/hook.cc
    will/iomanager.cc
    will/log.cc
    will/mutex.cc
    will/scheduler.cc
    will/socket_stream.cc
    will/socket.cc
    will/stream.cc
    will/tcp_server.cc
    will/thread.cc
    will/timer.cc
    will/util.cc
    will/http/http_parser.cc 
    will/http/http_server.cc 
    will/http/http_session.cc
    will/http/http.cc 
    will/http/servlet.cc
    will/http/http11_parser.rl.cc
    will/http/httpclient_parser.rl.cc
    )

add_library(will SHARED ${LIB_SRC})

set(LIBS
    will
    pthread
    dl
)

if(BUILD_TEST)
will_add_executable(test_http "tests/perf_test_http.cc" will "${LIBS}")
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)