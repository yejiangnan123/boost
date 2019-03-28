# boost http
## http_server
* advanced_server.cpp
```
高级http服务器，支持websocket，长连接，服务器可以主动给客户端推送消息
增加定时器，保持连接是活跃的
```
* http_server_async.cpp
```
简单的异步http服务器
```
* http_server_coro.cpp
```
基于协程的http服务器
协程特点是使异步编程更简单，可读性好，同时拥有异步的高效率和同步的高可读性
```
* http_server_fast.cpp
```
快速服务器，每种请求进行分类，不会做多余的处理，所以处理速度比较快
增加请求超时处理，一个请求60s没有处理，则关闭连接
```
* http_server_flex.cpp
* http_server_small.cpp
* http_server_stackless.cpp
* http_server_sync.cpp
## http_client
* http_client_async.cpp
