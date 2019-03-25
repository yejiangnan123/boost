---------------------------------------------------------------------------------
boost/asio/deadline_timer.hpp   截止定时器
boost/asio.hpp
-----------------------------------------------------------------------------------
// 同步定时器，阻塞
boost::asio::deadline_timer timer(io_context);
timer.expires_from_now(boost::posix_time::seconds(5));
timer.wait();
--------------------------------
//异步定时器
void handler(const boost::system::error_code& error)
{
  if (!error)
  {
  }
}
boost::asio::deadline_timer timer(io_context,
    boost::posix_time::time_from_string("2005-12-07 23:59:59.000"));
timer.async_wait(handler);
-----------------------------------
//取消异步等待任务
void on_some_event()
{
  if (my_timer.expires_from_now(seconds(5)) > 0)
  {
    my_timer.async_wait(on_timeout);
  }
  else
  {
    // Too late, timer has already expired!
  }
}

void on_timeout(const boost::system::error_code& e)
{
  if (e != boost::asio::error::operation_aborted)
  {
    // Timer was not cancelled, take necessary action.
  }
}
--------------------------------------------------------------------------------------------------
boost/asio/high_resolution_timer.hpp  高精度定时器
--------------------------------------------------------------------------------------------------
// 同步等待，阻塞
boost::asio::steady_timer timer(io_context);
timer.expires_after(std::chrono::seconds(5));
timer.wait();
-------------------------------------
//异步等待
void handler(const boost::system::error_code& error)
{
  if (!error)
  {
    // Timer expired.
  }
}
boost::asio::steady_timer timer(io_context,
    std::chrono::steady_clock::now() + std::chrono::seconds(60));
timer.async_wait(handler);
-----------------------------------
//取消等待
void on_some_event() {
  if (my_timer.expires_after(seconds(5)) > 0){
    my_timer.async_wait(on_timeout);
  }
  else{
  }
}

void on_timeout(const boost::system::error_code& e){
  if (e != boost::asio::error::operation_aborted){
  }
}
------------------------------------------------------------------------------------------
boost/asio/steady_timer.hpp   稳定定时器
------------------------------------------------------------------------------------------
//同步等待，阻塞
boost::asio::steady_timer timer(io_context);
timer.expires_after(std::chrono::seconds(5));
timer.wait();
---------------------------------------
//异步等待
void handler(const boost::system::error_code& error)
{
  if (!error)
  {
    // Timer expired.
  }
}
boost::asio::steady_timer timer(io_context,
    std::chrono::steady_clock::now() + std::chrono::seconds(60));
timer.async_wait(handler);
--------------------------------------
//取消等待
void on_some_event(){
  if (my_timer.expires_after(seconds(5)) > 0){
    my_timer.async_wait(on_timeout);
  }
  else{
    // Too late, timer has already expired!
  }
}

void on_timeout(const boost::system::error_code& e){
  if (e != boost::asio::error::operation_aborted){
    // Timer was not cancelled, take necessary action.
  }
}
----------------------------------------------------------------------------------------
boost/asio/system_timer.hpp  系统定时器
-----------------------------------------------------------------------------------------
// 同步等待，阻塞
boost::asio::steady_timer timer(io_context);
timer.expires_after(std::chrono::seconds(5));
timer.wait();
---------------------------------
//异步等待
void handler(const boost::system::error_code& error)
{
  if (!error)
  {
  }
}
boost::asio::steady_timer timer(io_context,
    std::chrono::steady_clock::now() + std::chrono::seconds(60));
timer.async_wait(handler);
----------------------------------
//取消等待
void on_some_event(){
  if (my_timer.expires_after(seconds(5)) > 0){
    my_timer.async_wait(on_timeout);
  }
  else{
  }
}
void on_timeout(const boost::system::error_code& e){
  if (e != boost::asio::error::operation_aborted){
  }
}
