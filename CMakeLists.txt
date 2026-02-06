#指定最低版本号
cmake_minimum_required(VERSION 3.16)
project(Fcw_muduo VERSION 1.0 LANGUAGES CXX)

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
            ./soure/Fcw_Channel.cc)
add_executable(Fcw_muduo ${SOURCES})

#链接pthread库
target_link_libraries(Fcw_muduo pthread)
