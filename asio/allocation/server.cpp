#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
//-----------------------------------------------------------------
//内存管理
class handler_memory
{
public:
  handler_memory()
    : in_use_(false)
  {
  }

  handler_memory(const handler_memory&) = delete;   // delete作用：禁止使用该函数
  handler_memory& operator=(const handler_memory&) = delete;

  void* allocate(std::size_t size)
  {
    if (!in_use_ && size < sizeof(storage_))
    {
      in_use_ = true;
      return &storage_;
    }
    else
    {
      return ::operator new(size);   // 定制自己的new
    }
  }

  void deallocate(void* pointer)
  {
    if (pointer == &storage_)
    {
      in_use_ = false;
    }
    else
    {
      ::operator delete(pointer);   //定制自己的delete
    }
  }

private:
  typename std::aligned_storage<1024>::type storage_;   //分配字节空间
  bool in_use_;
};
//----------------------------------------------------------
//分配器
template <typename T>
class handler_allocator
{
public:
  using value_type = T;

  explicit handler_allocator(handler_memory& mem)
    : memory_(mem)
  {
  }

  template <typename U>
  handler_allocator(const handler_allocator<U>& other) noexcept   //noexcept：禁止抛出异常
    : memory_(other.memory_)
  {
  }

  bool operator==(const handler_allocator& other) const noexcept
  {
    return &memory_ == &other.memory_;
  }

  bool operator!=(const handler_allocator& other) const noexcept
  {
    return &memory_ != &other.memory_;
  }

  T* allocate(std::size_t n) const
  {
    return static_cast<T*>(memory_.allocate(sizeof(T) * n));
  }

  void deallocate(T* p, std::size_t /*n*/) const
  {
    return memory_.deallocate(p);
  }

private:
  template <typename> friend class handler_allocator;
  handler_memory& memory_;
};
//---------------------------------------------------------------
//注册：内存分配器+处理函数
template <typename Handler>
class custom_alloc_handler
{
public:
  using allocator_type = handler_allocator<Handler>;

  custom_alloc_handler(handler_memory& m, Handler h)
    : memory_(m),
      handler_(h)
  {
  }

  allocator_type get_allocator() const noexcept
  {
    return allocator_type(memory_);
  }

  template <typename ...Args>  //可变参数
  void operator()(Args&&... args)
  {
    handler_(std::forward<Args>(args)...);
  }

private:
  handler_memory& memory_;
  Handler handler_;
};
//-------------------------------------------------------------
//一个模板函数，返回 内存+处理函数 
template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
    handler_memory& m, Handler h)
{
  return custom_alloc_handler<Handler>(m, h);
}
//-----------------------------------------------------------
//管理tcp连接
class session
  : public std::enable_shared_from_this<session>  //继承智能指针，允许向函数传递本对象为智能指针
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());   //获取本对象的智能指针
    socket_.async_read_some(boost::asio::buffer(data_),   //异步读数据
        make_custom_alloc_handler(handler_memory_,
          [this, self](boost::system::error_code ec, std::size_t length)
          {
            if (!ec)
            {
              do_write(length);
            }
          }));
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length), //异步写数据
        make_custom_alloc_handler(handler_memory_,
          [this, self](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (!ec)
            {
              do_read();
            }
          }));
  }
  tcp::socket socket_;
  std::array<char, 1024> data_;
  handler_memory handler_memory_;
};
//-----------------------------------------------------------------
//tcp监听服务器
class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))  //监听本机ip
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(              //异步接收客户端连接
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();  //创建一个连接并启动
          }

          do_accept();  //继续监听客户端连接
        });
  }

  tcp::acceptor acceptor_;
};
//--------------------------------------
//主函数
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
    server s(io_context, std::atoi(argv[1]));
    io_context.run();  // 运行，开始处理任务，本函数将会阻塞，如果没有待处理的任务，这个函数将会退出，接收异步通信
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
