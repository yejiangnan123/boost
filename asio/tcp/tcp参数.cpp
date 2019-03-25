----------------------------------------------------
boost/asio/ip/tcp.hpp
----------------------------------------------------
boost::asio::ip::tcp::no_delay   用于禁用Nagle算法的套接字选项。
--------------------------------
设置参数
boost::asio::ip::tcp::socket socket(io_context);
...
boost::asio::ip::tcp::no_delay option(true);
socket.set_option(option);
---------------------------------
//获取参数
boost::asio::ip::tcp::socket socket(io_context);
...
boost::asio::ip::tcp::no_delay option;
socket.get_option(option);
bool is_set = option.value();
