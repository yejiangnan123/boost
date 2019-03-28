# streambuf
* 头文件
boost/asio/streambuf.hpp
* 写数据
```
boost::asio::streambuf b;
std::ostream os(&b);
os << "Hello, World!\n";
size_t n = sock.send(b.data());
b.consume(n); // 移除n个字节
```
* 读数据
```
boost::asio::streambuf b;
boost::asio::streambuf::mutable_buffers_type bufs = b.prepare(512);//指定缓冲区列表大小
size_t n = sock.receive(bufs);//接收数据
b.commit(n);//提交数据
std::istream is(&b);
std::string s;
is >> s;//读数据
```
# buffer
* 头文件
boost/asio/buffer.hpp
* 只读
```
//只读buffer
boost::asio::const_buffer b1 = ...;
std::size_t s1 = b1.size();
const unsigned char* p1 = static_cast<const unsigned char*>(b1.data());
```
* 可读写buffer
```
boost::asio::mutable_buffer b1 = ...;
std::size_t s1 = b1.size();
unsigned char* p1 = static_cast<unsigned char*>(b1.data());
```
* buffer函数
```
char d1[128];
std::vector<char> d2(128);
boost::array<char, 128> d3;

boost::array<mutable_buffer, 3> bufs1 = {
  boost::asio::buffer(d1),
  boost::asio::buffer(d2),
  boost::asio::buffer(d3) };
bytes_transferred = sock.receive(bufs1);

std::vector<const_buffer> bufs2;
bufs2.push_back(boost::asio::buffer(d1));
bufs2.push_back(boost::asio::buffer(d2));
bufs2.push_back(boost::asio::buffer(d3));
bytes_transferred = sock.send(bufs2);
```
* 复制
```
vector<const_buffer> buffers = ...;
vector<unsigned char> data(boost::asio::buffer_size(buffers));
boost::asio::buffer_copy(boost::asio::buffer(data), buffers);、
```
