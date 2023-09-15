## 介绍

本项目是linux下一个基于c++23和协程的liburing的异步高并发Proactor模式服务器，利用liburing的特性实现真正的异步收发

## 性能测试

Arch WSL  
8核16线程，16G内存  
利用[wrk](https://github.com/wg/wrk)测试，测试结果如下  
![image](test/test.png)

RPS:177万（每秒处理的请求数量）

wrk是一款现代HTTP基准测试工具，在单核CPU上运行时能够产生巨大的负载。它将多线程设计与可扩展的事件通知系统（如epoll和kqueue）相结合

## 登录注册演示

![image](test/test.gif)

## 并发模型

模型是one scheduler per thread，各线程间相互独立，互不干扰

## 调度器

每个调度器都持有一个io_uring实例，buf_ring实例，服务器实例，定时器实例和数据库连接，并且通过调度协程的暂停与恢复实现协程的调度

## io_uring

使用raii包装liburing中的各个资源与函数，实现生命周期的良好管理

## 协程

由于目前c++20的协程还只是底层实现，并不好用，所以对当前协程进行了封装，实现了协程的创建与销毁、暂停与恢复、调度和异常处理

## 异步日志

日志利用异步的无锁队列实现，前端只需往队列中添加日志，后端会自动将日志写入到log.log文件，真正实现异步写入且线程安全

## http解析

利用状态机解析http报文，支持长连接，支持http1.1，支持GET和HEAD和POST请求，支持静态资源请求，支持br压缩，利用mysql支持登陆与注册，支持网页，图片和视频的请求，使用c++20ranges的split的惰性求值提高解析速度

## 定时器

利用时间轮实现定时器，每个调度器都有一个时间轮，每个连接都有一个定时任务，定时器的精度为1s，会自动处理超时的连接并释放

## 数据库

数据库使用mariadb，通过对mariadb c api的简单封装，实现了数据库的连接与断开，查询和插入

在编译前需要在mariadb中创建数据库和表，如下

```shell
create database webServer;
use webServer;
CREATE TABLE `users` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `password` varchar(32) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

mariadb的用户名和密码，在Database类的构造函数中修改

## 环境

gcc13以上版本，cmake3.27.5以上版本，liburing2.4以上版本，linux内核版本6.1以上，ninja，brotli库，mariadb

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

