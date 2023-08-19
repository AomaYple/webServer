## 项目介绍

本项目是一个基于c++23和liburing的异步高并发Proactor模式服务器，利用liburing的特性实现真正的
异步收发

## 项目测试

Arch Linux  
8核16线程，16G内存  
利用[wrk](https://github.com/wg/wrk)测试，测试结果如下  
![image](test/test.png)

RPS:144万（每秒处理的请求数量）

wrk是一款现代HTTP基准测试工具，在单核CPU上运行时能够产生巨大的负载。它将多线程设计与可扩展的事件通知系统（如epoll和kqueue）相结合。

## 演示视频

[![Login](https://i.imgur.com/vKb2F1B.png)](https://github.com/AomaYple/WebServer/tree/main/test.mp4)

## 项目所需环境

gcc13.1以上版本，cmake3.22以上版本，libuing2.4以上版本，linux内核版本6.1以上，ninja，brotli库

## 项目编译

```shell 
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cd build
ninja
```

## 项目运行

```shell
cd build/WebServer
./WebServer
```

## 异步日志的实现

日志利用异步的无锁队列实现，前端只需往队列中push日志，后端会自动将日志写入到log.log文件，真正实现异步写入
且线程安全

## http报文解析

利用状态机解析http报文，支持长连接，支持http1.1，支持GET和HEAD和POST请求，支持静态资源请求，支持br压缩，利用mysql支持登陆与注册，支持网页，图片和视频的请求

## 并发模型

每个线程都独立的持有一个io_uring实例和server实例，之间互不干扰，每个线程都是一个独立的事件循
环，不用为了线程间同步用锁降低性能，利用SO_REUSEADDR和SO_REUSEPORT让多个独立的server绑定到
同一地址和端口上，新连接随机分配到一个server上，然后处理连接的读写和释放

## 定时器

利用时间轮实现定时器，每个线程都有一个时间轮，每个连接都有一个定时器，定时器的精度为1s，会自动
处理超时的连接并释放
