#搜索这个目录，当前开发环境，没有找到mysql库
link_directories(/usr/lib64/mysql)

#指定最低版本号
cmake_minimum_required(VERSION 3.16)
project(music VERSION 1.0 LANGUAGES CXX)

#指定C++标准（根据你的代码实际用到的版本选，比如C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制要求标准，否则报错

#源文件列表
set(SOURCES ./soure/main.cc 
            ./soure/Fcw_Util.cc 
            ./soure/Fcw_TimerWheel.cc 
            ./soure/Fcw_Socket.cc 
            ./soure/Fcw_Poller.cc 
            ./soure/Fcw_NoBlock.cc 
            ./soure/Fcw_Http.cc 
            ./soure/Fcw_EventLoop.cc 
            ./soure/Fcw_Connection.cc 
            ./soure/Fcw_Acceptor.cc 
            ./soure/Fcw_Buffer.cc 
            ./soure/Fcw_Channel.cc
            ./soure/Fcw_Routes.cc
            ./soure/Tasks_table.cc
            ./soure/User_table.cc)
add_executable(HttpServer ${SOURCES})

# 链接所需库
target_link_libraries(HttpServer
    pthread                     # 多线程
    mysqlclient                 # MySQL C API
    jsoncpp                     # JSON 解析
    uuid                        # UUID 生成
    ssl                         # OpenSSL (SHA256)
    crypto                      # OpenSSL 加密库
    hiredis                     # redis c API
)