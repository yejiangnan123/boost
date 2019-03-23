#include <cstdlib>
#include <functional>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;
//tcp连接对象
class session : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket, boost::asio::ssl::context& context)
    : socket_(std::move(socket), context)
  {
  }

  void start()
  {
    do_handshake();
  }

private:
  void do_handshake()
  {
    auto self(shared_from_this());
    socket_.async_handshake(boost::asio::ssl::stream_base::server, 
        [this, self](const boost::system::error_code& error)   //异步握手
        {
          if (!error)
          {
            do_read();  //握手完成开始读
          }
        });
  }

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        [this, self](const boost::system::error_code& ec, std::size_t length)   //异步读
        {
          if (!ec)
          {
            do_write(length);  //读完回写
          }
        });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        [this, self](const boost::system::error_code& ec,
          std::size_t /*length*/)    //异步写
        {
          if (!ec)
          {
            do_read(); //写完继续读
          }
        });
  }

  boost::asio::ssl::stream<tcp::socket> socket_;
  char data_[1024];
};
//服务器
class server
{
public:
  server(boost::asio::io_context& io_context, unsigned short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      context_(boost::asio::ssl::context::sslv23)
  {
    context_.set_options(
        boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::single_dh_use);   //设置参数
    context_.set_password_callback(std::bind(&server::get_password, this));  //设置密码
    context_.use_certificate_chain_file("server.pem");    //证书文件 
    context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);//私钥文件
    context_.use_tmp_dh_file("dh2048.pem");   

    do_accept();  /开始监听
  }

private:
  std::string get_password() const   //密码
  {
    return "test";
  }

  void do_accept()
  {
    acceptor_.async_accept(
        [this](const boost::system::error_code& error, tcp::socket socket)  异步监听客户端连接
        {
          if (!error)
          {
            std::make_shared<session>(std::move(socket), context_)->start();  //创建连接对象
          }

          do_accept(); //继续监听
        });
  }

  tcp::acceptor acceptor_;
  boost::asio::ssl::context context_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    using namespace std; // For atoi.
    server s(io_context, atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
