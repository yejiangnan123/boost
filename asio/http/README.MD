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
```
增加ssl功能，也就是连接的时候增加握手环节，握手的功能是验证证书的正确性
```
* http_server_small.cpp
```
小，是这个服务器的特点，只有一个线程，代码小，结构简单
流程：监听,读，写,关闭
超时处理：60s内必须处理完毕，否则直接关闭连接
```
* http_server_stackless.cpp
```
监听和处理请求都用了协程
```
* http_server_sync.cpp
```
最简单的方式，同步处理请求，每次新的连接过来，创建一个线程去处理连接请求。
可以用线程池简单优化，适用于短连接
```
## http_client
* http_client_async.cpp
```
最简单的异步客户端，连接，写，读，关闭
```
