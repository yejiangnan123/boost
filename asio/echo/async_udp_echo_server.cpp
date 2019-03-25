//boost::asio::io_context io_context;
//udp::socket socket_;
//udp::endpoint sender_endpoint_;
//socket_(io_context, udp::endpoint(udp::v4(), port))
//socket_.async_receive_from(
//boost::asio::buffer(data_, max_length), sender_endpoint_,[this](boost::system::error_code ec, std::size_t bytes_recvd)
//socket_.async_send_to(boost::asio::buffer(data_, length), sender_endpoint_,[this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
//io_context.run();
#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>

using boost::asio::ip::udp;
//udp服务器
class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : socket_(io_context, udp::endpoint(udp::v4(), port))
  {
    do_receive();
  }

  void do_receive() //读
  {
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_recvd)
        {
          if (!ec && bytes_recvd > 0)
          {
            do_send(bytes_recvd);
          }
          else
          {
            do_receive();
          }
        });
  }

  void do_send(std::size_t length) //写
  {
    socket_.async_send_to(
        boost::asio::buffer(data_, length), sender_endpoint_,
        [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
        {
          do_receive();
        });
  }

private:
  udp::socket socket_;
  udp::endpoint sender_endpoint_;
  enum { max_length = 1024 };
  char data_[max_length];
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_udp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
