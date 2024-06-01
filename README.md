## 介绍

本项目是linux上一个基于c++和io_uring的异步高并发Proactor模式服务器

## 日志

利用io_uring的异步io和linux O_APPEND特性实现了异步且线程安全的高性能日志系统，支持多种日志级别和提供详细的日志信息

## json

基于递归下降实现了对json的解析和生成，支持近乎所有的json格式，用于支持http请求和响应的解析和生成

## 协程

包装c++20协程的coroutine，实现了Awaiter和Task，简化异步编程

## io_uring

利用io_uring实现了高性能的异步io，支持多个io操作的批量提交，减少系统调用次数，提高性能

## 定时器

基于层级时间轮实现定时器，定时器的精度为1s，支持极大时间范围，会自动处理超时的文件描述符，节省服务器资源

## 数据库

数据库使用mariadb存储用户的信息，使用前需要创建数据库和表，如下：

```sql'
create database webServer;

use webServer;

CREATE TABLE `users` (
`id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
`password` varchar(32) NOT NULL,
PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

## http

支持http1.1、长连接和br压缩，支持GET、HEAD和POST请求，支持请求网页、图片和视频，支持登录和注册

## 调度器

基于协程实现了一个简单的调度器，支持协程的创建、销毁、挂起和唤醒，程序会根据cpu核心数创建相应数量的调度器，每个调度器互相独立，互不干扰

## 环境

gcc14以上，cmake，ninja，liburing2.4以上，brotli，mariadb

## 编译

```shell 
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cd build
ninja
```

## 运行

```shell
cd build/webServer
./webServer
```

## 性能测试

Arch WSL  
8核  
利用[wrk](https://github.com/wg/wrk)测试，测试结果如下  
![image](show/test.png)

RPS:167万（每秒处理的请求数量）

wrk是一款现代HTTP基准测试工具，在单核CPU上运行时能够产生巨大的负载。它将多线程设计与可扩展的事件通知系统（如epoll和kqueue）相结合

## 演示

![image](show/show.gif)
