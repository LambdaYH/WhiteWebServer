# WhiteWebServer
Linux环境下的轻量级webserver

* 使用Epoll和线程池实现多线程Reactor模式的服务器模型。
* 使用小根堆实现的定时器来处理超时连接
* 使用双缓冲区的异步日志系统
* 实现解析静态HTTP请求。

## Requirements
```
sudo apt install libjsoncpp-dev
```

## Build
```
git clone https://github.com/LambdaYH/WhiteWebServer.git
cd WhiteWebServer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Todo
- [x] 支持对不完整请求的处理
- [x] 支持keep-alive
- [ ] 支持gzip
- [x] 异步日志
- [ ] 添加proxy。
- [x] 支持解析post请求。
- [ ] 支持websocket。
- [x] 支持配置文件。

## 致谢
Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)

[@markparticle](https://github.com/markparticle/WebServer)