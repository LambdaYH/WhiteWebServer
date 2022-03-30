# WhiteWebServer
Linux环境下的轻量级webserver

* 使用Epoll和线程池实现多线程Reactor模式的服务器模型。
* 使用小根堆实现的定时器来处理超时连接
* 日志系统
* 实现解析HTTP请求。

## Bugs to fix
- [ ] 处理好keep-alive
- [ ] 对不完整请求进行完善的处理

## Todo
- [ ] 异步日志
- [ ] 添加proxy。
- [ ] 支持post请求。
- [ ] 支持websocket。

## 致谢
Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)

[@markparticle](https://github.com/markparticle/WebServer)