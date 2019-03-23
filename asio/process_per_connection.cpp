#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using boost::asio::ip::tcp;
//服务器
class server
{
public:
  server(boost::asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      signal_(io_context, SIGCHLD),
      acceptor_(io_context, {tcp::v4(), port}),
      socket_(io_context)
  {
    wait_for_signal();
    accept();
  }

private:
  void wait_for_signal()  异步等待信号
  {
    signal_.async_wait(
        [this](boost::system::error_code /*ec*/, int /*signo*/)
        {
          if (acceptor_.is_open())
          {
            int status = 0;
            while (waitpid(-1, &status, WNOHANG) > 0) {}

            wait_for_signal();
          }
        });
  }
//异步接收
  void accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket new_socket)
        {
          if (!ec)
          {
            socket_ = std::move(new_socket);

            io_context_.notify_fork(boost::asio::io_context::fork_prepare);//通知，我要分叉

            if (fork() == 0)
            {
              io_context_.notify_fork(boost::asio::io_context::fork_child);//子进程通知，我们完成分叉

              acceptor_.close();//关键监听器，不接收新连接
              signal_.cancel();//对信号不感兴趣，取消信号

              read();//子进程只进行读，不监听
            }
            else
            {

              io_context_.notify_fork(boost::asio::io_context::fork_parent);//父进程通知，我们完成分叉或者失败

              socket_.close();//父进程关闭套接字，在子进程有效

              accept();//父进程继续监听
            }
          }
          else
          {
            std::cerr << "Accept error: " << ec.message() << std::endl;
            accept();
          }
        });
  }

  void read()
  {
    socket_.async_read_some(boost::asio::buffer(data_),
        [this](boost::system::error_code ec, std::size_t length)  //异步读
        {
          if (!ec)
            write(length);
        });
  }

  void write(std::size_t length)
  {
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        [this](boost::system::error_code ec, std::size_t /*length*/)  //异步写
        {
          if (!ec)
            read();    //写完继续读
        });
  }

  boost::asio::io_context& io_context_;
  boost::asio::signal_set signal_;  //信号集合
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::array<char, 1024> data_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: process_per_connection <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    using namespace std; // For atoi.
    server s(io_context, atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}
