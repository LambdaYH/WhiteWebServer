# WhiteWebServer

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/ca0cb125089f4dc29e02df1223c76499)](https://www.codacy.com/gh/LambdaYH/WhiteWebServer/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=LambdaYH/WhiteWebServer&amp;utm_campaign=Badge_Grade)

Linux环境下的轻量级webserver

-   使用Epoll和线程池实现多线程Reactor模式的服务器模型。
-   使用小根堆实现的定时器来处理超时连接。
-   使用双缓冲区的异步日志系统。
-   实现解析静态HTTP请求。
-   支持基本的反向代理设置。
-   支持配置文件。

## Requirements

    cmake

## Build

    git clone https://github.com/LambdaYH/WhiteWebServer.git
    cd WhiteWebServer
    mkdir build && cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

## Todo

-   [x] 支持对不完整请求的处理
-   [x] 支持keep-alive
-   [ ] 支持gzip
-   [x] 异步日志
-   [x] 支持reverse proxy（还有bug）
-   [x] 支持解析post请求
-   [ ] 支持websocket
-   [x] 支持配置文件

## 致谢

Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)

[@markparticle](https://github.com/markparticle/WebServer)
