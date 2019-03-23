//tcp监听客户端连接，连接成功后给客户端发送系统当前时间
//定义一个只读缓存，tcp发送这个缓存数据给客户端
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <ctime>

using boost::asio::ip::tcp;
//----------------------------------------------------
// 共享只读buffer
class shared_const_buffer
{
public:
  explicit shared_const_buffer(const std::string& data)
    : data_(new std::vector<char>(data.begin(), data.end())),
      buffer_(boost::asio::buffer(*data_))
  {
  }

  typedef boost::asio::const_buffer value_type;
  typedef const boost::asio::const_buffer* const_iterator;
  const boost::asio::const_buffer* begin() const { return &buffer_; }
  const boost::asio::const_buffer* end() const { return &buffer_ + 1; }

private:
  std::shared_ptr<std::vector<char> > data_;
  boost::asio::const_buffer buffer_;
};
//---------------------------------------------
//tcp连接管理
class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
    std::cout<<"session \n";
  }

  void start()
  {
    do_write();
  }

private:
  void do_write()
  {
    std::time_t now = std::time(0);   
    shared_const_buffer buffer(std::ctime(&now));  //当前时间

    auto self(shared_from_this());
    boost::asio::async_write(socket_, buffer,   //异步发送当前时间
        [this, self](boost::system::error_code /*ec*/, std::size_t /*length*/)
        {
          //std::cout<<"async_write \n";
          //do_write();   //这里打开将会一直循环异步调用do_write函数,我们测试只发送一次
        });
  }

  // The socket used to communicate with the client.
  tcp::socket socket_;
};
//---------------------------------------------------
//tcp监听服务器
class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};
//---------------------------------------
//主函数
int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: reference_counted <port>\n";
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
