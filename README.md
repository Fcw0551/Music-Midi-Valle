# Music-Midi-Valle
针对MIDI-VALLE钢琴曲合成模型，开发web端，实现异步队列进行任务解耦，采用CMake在不同平台下快速构建项目

#以下是整个的项目结构
Music-Midi-Valle/
├── build/                  		# CMake 构建目录（编译中间文件、可执行文件）
├── include/                		# C++ 头文件
│   ├── Fcw_HttpServer.hpp  	# HTTP 服务层：路由注册、静态文件处理、请求分发
│   ├── Fcw_Connection.hpp 	# TCP 连接封装：读写缓冲区、连接状态、回调管理
│   ├── Fcw_TcpServer.hpp   	# TCP 服务器：监听端口、管理连接池、工作线程池
│   ├── Fcw_Poller.hpp      	# epoll 封装：事件添加、修改、删除
│   ├── Fcw_Channel.hpp     	# 事件通道：绑定文件描述符与回调函数
│   ├── Fcw_EventLoop.hpp   	# 事件循环：定时器、任务队列、跨线程调度
│   ├── Fcw_Buffer.hpp      	# 非连续缓冲区：支持读写偏移和数据追加
│   ├── Fcw_Socket.hpp      	# 套接字操作：非阻塞读写、地址绑定
│   ├── Fcw_Util.hpp        		# 工具函数：SHA256、UUID 生成、文件读写、MIME 解析等
│   ├── Fcw_Routes.hpp      	# 路由处理函数声明
│   ├── Fcw_NoBlock.hpp     	# 非阻塞辅助函数
│   ├── Fcw_TimerWheel.hpp   # 时间轮定时器（用于连接超时管理）
│   ├── Fcw_Any.hpp         	# 类型擦除的上下文存储
│   ├── Tasks_table.hpp     	# 任务表数据库操作接口
│   ├── User_table.hpp      	# 用户表数据库操作接口
│   └── Fcw_Log.hpp        	 	# 日志宏定义
├── scripts/                		# Python 脚本
│   ├── convert_worker.py   	# 图片转 MIDI 消费者（Audiveris + music21）
│   ├── inference_worker.py 	# MIDI 推理消费者（MIDI-VALLE 模型）
├── soure/                  		# C++ 源文件（与头文件对应实现）
│   ├── main.cc             		# 程序入口：初始化 Redis/MySQL、启动 HTTP 服务
│   ├── Fcw_Routes.cc       	# 具体业务逻辑实现（登录、注册、任务提交、状态查询、管理员功能）
│   ├── Fcw_Connection.cc   	# 连接生命周期管理（读写事件、关闭流程、智能指针防护）
│   ├── Fcw_TcpServer.cc    	# TCP 服务器实现
│   ├── Fcw_Poller.cc       		# Poller 实现
│   ├── Fcw_Channel.cc      	# Channel 实现
│   ├── Fcw_EventLoop.cc    	# EventLoop 实现
│   ├── Fcw_Buffer.cc       		# Buffer 实现
│   ├── Fcw_Socket.cc       	# Socket 实现
│   ├── Fcw_Util.cc         		# 工具函数实现
│   ├── Fcw_NoBlock.cc      	# 非阻塞实现
│   ├── Fcw_TimerWheel.cc   	# 时间轮实现
│   ├── Fcw_Any.cc          		# Any 实现
│   ├── Tasks_table.cc      		# 任务表数据库访问实现
│   └── User_table.cc      		# 用户表数据库访问实现
├── wwwroot/                		# 前端静态资源
│   ├── index.html          		# 登录界面
│   └── main.html           		# 主页面（用户端/管理员端）
├── CMakeLists.txt          		# CMake 构建配置
└── README.md               	# 本文件

