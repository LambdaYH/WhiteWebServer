# WhiteWebServer
Linux环境下的轻量级webserver

* 使用Epoll和线程池实现多线程Reactor模式的服务器模型。
* 使用小根堆实现的定时器来处理超时连接
* 日志系统
* 实现解析静态HTTP请求。

## Todo
- [x] 支持keep-alive
- [ ] 异步日志
- [ ] 添加proxy。
- [ ] 支持post请求。
- [ ] 支持websocket。
- [ ] 支持配置文件。

## 致谢
Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)

[@markparticle](https://github.com/markparticle/WebServer)