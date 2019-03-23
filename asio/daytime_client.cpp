//异步操作和异步结果future结合使用
#include <array>
#include <future>
#include <iostream>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_future.hpp>

using boost::asio::ip::udp;

void get_daytime(boost::asio::io_context& io_context, const char* hostname)
{
  try
  {
    udp::resolver resolver(io_context);

    std::future<udp::resolver::results_type> endpoints =
      resolver.async_resolve(
          udp::v4(), hostname, "daytime",
          boost::asio::use_future);

    udp::socket socket(io_context, udp::v4());

    std::array<char, 1> send_buf  = {{ 0 }};
    std::future<std::size_t> send_length =             //用future变量保存异步操作返回值，这里是异步发送
      socket.async_send_to(boost::asio::buffer(send_buf),
          *endpoints.get().begin(), // ... until here. This call may block.
          boost::asio::use_future);

    send_length.get();   //获取异步操作的结果

    std::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    std::future<std::size_t> recv_length =   //异步接收，并且接收返回结果
      socket.async_receive_from(
          boost::asio::buffer(recv_buf),
          sender_endpoint,
          boost::asio::use_future);
          
    std::cout.write(
        recv_buf.data(),
        recv_length.get()); 
  }
  catch (std::system_error& e)
  {
    std::cerr << e.what() << std::endl;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }
    boost::asio::io_context io_context;
    auto work = boost::asio::make_work_guard(io_context);//没有任务，run也不会停止
    std::thread thread([&io_context](){ io_context.run(); });//创建一个线程，执行run，这里没有任务run也不会停止，因为上面设置了work属性

    get_daytime(io_context, argv[1]);

    io_context.stop();
    thread.join();
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
