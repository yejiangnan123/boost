#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
#include <array>
#include <ctime>
#include <iostream>
#include <syslog.h>
#include <unistd.h>

using boost::asio::ip::udp;
//-------------------------------------------
//udp服务器
class udp_daytime_server
{
public:
  udp_daytime_server(boost::asio::io_context& io_context)
    : socket_(io_context, {udp::v4(), 13})
  {
    receive();
  }

private:
  void receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t /*n*/)   //异步接收数据
        {
          if (!ec)
          {
            using namespace std; // For time_t, time and ctime;
            time_t now = time(0);
            std::string message = ctime(&now);

            boost::system::error_code ignored_ec;
            socket_.send_to(boost::asio::buffer(message),
                remote_endpoint_, 0, ignored_ec);    //发送数据
          }

          receive();   //继续接收
        });
  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  std::array<char, 1> recv_buffer_;
};

int main()
{
  try
  {
    boost::asio::io_context io_context;
    udp_daytime_server server(io_context);//先启动服务器

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);//注册信号
    signals.async_wait(
        [&](boost::system::error_code /*ec*/, int /*signo*/)
        {
          io_context.stop();
        });  //异步等待信号，关闭服务器

    io_context.notify_fork(boost::asio::io_context::fork_prepare);//通知io_context，我将成为守护进程

    if (pid_t pid = fork())创建进程
    {
      if (pid > 0)
      {
        exit(0);//退出父进程
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "First fork failed: %m");
        return 1;//出错
      }
    }

    setsid();  //设置子进程为领头进程
    chdir("/");//切换到根目录
    umask(0);//不想失去文件权限，清除掩码

    if (pid_t pid = fork()) //又创建一个进程
    {
      if (pid > 0)
      {
        exit(0);// 父进程退出
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
        return 1; //错误
      }
    }

    close(0); //关闭3个标准流
    close(1);
    close(2);

    if (open("/dev/null", O_RDONLY) < 0) //守护进程不应该有输入，重定向到/dev/null
    {
      syslog(LOG_ERR | LOG_USER, "Unable to open /dev/null: %m");
      return 1;
    }

    const char* output = "/tmp/asio.daemon.out";   //将标准输出发送到日志文件
    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (open(output, flags, mode) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to open output file %s: %m", output);
      return 1;
    }

    if (dup(1) < 0)  //将标准错误发送到同一文件
    {
      syslog(LOG_ERR | LOG_USER, "Unable to dup output descriptor: %m");
      return 1;
    }

    io_context.notify_fork(boost::asio::io_context::fork_child);//通知io_context我们已经创建好了守护进程

    syslog(LOG_INFO | LOG_USER, "Daemon started");
    io_context.run();                                    //启动它
    syslog(LOG_INFO | LOG_USER, "Daemon stopped");
  }
  catch (std::exception& e)
  {
    syslog(LOG_ERR | LOG_USER, "Exception: %s", e.what());
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}
