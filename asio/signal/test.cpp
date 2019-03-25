---------------------------
boost/asio/signal_set.hpp
boost/asio.hpp
----------------------------
void handler(
    const boost::system::error_code& error,
    int signal_number) {
  if (!error){}
}
boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);//注册信号
signals.async_wait(handler);//注册信号处理函数
--------------------------------
//普通函数
void signal_handler(
    const boost::system::error_code& ec,
    int signal_number)
{
  ...
}
-----------------------------------
//对象
struct signal_handler
{
  ...
  void operator()(
      const boost::system::error_code& ec,
      int signal_number)
  {
    ...
  }
  ...
};
------------------------------------
 //匿名函数
my_signal_set.async_wait(
  [](const boost::system::error_code& ec,
    int signal_number)
  {
    ...
  });
------------------------------------
//std::bind
void my_class::signal_handler(
    const boost::system::error_code& ec,
    int signal_number)
{
  ...
}
...
my_signal_set.async_wait(
    std::bind(&my_class::signal_handler,
      this, std::placeholders::_1,
      std::placeholders::_2));
-----------------------------------------
//boost::bind
void my_class::signal_handler(
    const boost::system::error_code& ec,
    int signal_number)
{
  ...
}
...
my_signal_set.async_wait(
    boost::bind(&my_class::signal_handler,
      this, boost::asio::placeholders::error,
      boost::asio::placeholders::signal_number));
---------------------------------------------
