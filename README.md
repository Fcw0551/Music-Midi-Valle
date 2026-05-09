# MIDI-VALLE

针对 MIDI-VALLE 钢琴曲生成模型开发的 Web 系统，基于自研 Reactor 网络框架实现高并发 HTTP 服务，采用异步队列进行任务解耦，并结合 Redis / MySQL 实现任务管理与状态持久化，支持跨平台 CMake 构建。

---

# 项目特点

- 基于 epoll 实现 Reactor 网络模型
- 使用 One Loop Per Thread 提升并发处理能力
- 基于异步任务队列实现模型推理解耦
- 支持 Redis + MySQL 数据持久化
- 支持 HTTP 请求路由与静态资源访问
- 基于时间轮实现连接超时管理
- 支持跨平台 CMake 构建

---

# 系统架构

![输入图片说明](/imgs/2026-05-09/MB3GMYRv9AkPUvmv.png)

# 功能架构
![输入图片说明](/imgs/2026-05-09/jF3MlefT1M5HvCED.png)
# 项目结构

```text
Music-Midi-Valle/
├── build/                     # CMake 构建目录（编译中间文件、可执行文件）
├── include/                   # C++ 核心头文件
│
│ ├── Fcw_HttpServer.hpp       # HTTP 服务层：路由注册、静态文件处理、请求分发
│ ├── Fcw_Connection.hpp       # TCP 连接封装：缓冲区、连接状态、回调管理
│ ├── Fcw_TcpServer.hpp        # TCP 服务器：监听端口、连接池、线程池
│ ├── Fcw_Poller.hpp           # epoll 封装：事件添加、修改、删除
│ ├── Fcw_Channel.hpp          # 事件通道：绑定 fd 与回调函数
│ ├── Fcw_EventLoop.hpp        # Reactor 事件循环：任务队列、跨线程调度
│ ├── Fcw_Buffer.hpp           # 非连续缓冲区实现
│ ├── Fcw_Socket.hpp           # Socket 封装：非阻塞 IO、地址绑定
│ ├── Fcw_TimerWheel.hpp       # 时间轮定时器：连接超时管理
│ ├── Fcw_Any.hpp              # 类型擦除上下文存储
│ ├── Fcw_NoBlock.hpp          # 非阻塞辅助函数
│ ├── Fcw_Util.hpp             # 工具函数：SHA256、UUID、文件读写等
│ ├── Fcw_Routes.hpp           # HTTP 路由处理声明
│ ├── Tasks_table.hpp          # 任务表数据库接口
│ ├── User_table.hpp           # 用户表数据库接口
│ └── Fcw_Log.hpp              # 日志模块
│
├── source/                    # C++ 源文件实现
│
│ ├── main.cc                  # 程序入口：初始化 Redis/MySQL、启动 HTTP 服务
│ ├── Fcw_Routes.cc            # 登录、注册、任务提交、状态查询等业务逻辑
│ ├── Fcw_Connection.cc        # 连接生命周期管理
│ ├── Fcw_TcpServer.cc         # TCP Server 实现
│ ├── Fcw_Poller.cc            # Poller 实现
│ ├── Fcw_Channel.cc           # Channel 实现
│ ├── Fcw_EventLoop.cc         # EventLoop 实现
│ ├── Fcw_Buffer.cc            # Buffer 实现
│ ├── Fcw_Socket.cc            # Socket 实现
│ ├── Fcw_TimerWheel.cc        # 时间轮实现
│ ├── Fcw_Any.cc               # Any 实现
│ ├── Tasks_table.cc           # 任务数据库实现
│ └── User_table.cc            # 用户数据库实现
│
├── scripts/                   # Python 推理脚本
│
│ ├── convert_worker.py        # 图片转 MIDI 消费者（Audiveris + music21）
│ └── inference_worker.py      # MIDI-VALLE 推理消费者
│
├── wwwroot/                   # 前端静态资源
│ ├── index.html               # 登录页面
│ └── main.html                # 用户主页面 / 管理页面
│
├── CMakeLists.txt             # CMake 构建配置
└── README.md                  # 项目说明文档
```

---

# 核心模块说明

## Reactor 网络模块

基于 epoll 实现事件驱动网络模型，核心组件包括：

- EventLoop：事件循环与任务调度
- Poller：epoll 封装
- Channel：事件与回调绑定
- TcpServer：连接管理与线程池调度

采用：

```text
One Loop Per Thread
```

提升高并发场景下的网络事件处理能力。

---

## Buffer 模块

实现非连续缓冲区：

- 支持动态扩容
- 支持读写偏移
- 减少频繁内存拷贝

用于：

- HTTP 请求解析
- TCP 数据收发

---

## 时间轮模块

基于时间轮实现连接超时管理：

- 定时清理失活连接
- 降低大量定时器遍历开销

---

## 异步任务系统

基于异步队列进行任务解耦：

```text
HTTP 请求
    ↓
任务入队
    ↓
推理消费者处理
    ↓
结果持久化
```

避免模型推理阻塞主线程。

---

# 技术栈

- C++17
- Linux
- epoll
- Reactor
- 多线程
- Redis
- MySQL
- CMake
- Python
- MIDI-VALLE

---

# 构建方式

```bash
mkdir build
cd build
cmake ..
make -j
```

---

# 项目收获

- 理解 Reactor 高并发网络模型
- 理解 epoll 事件驱动机制
- 学习异步任务解耦设计
- 熟悉 Redis / MySQL 服务协作
- 熟悉高并发服务器架构设计
