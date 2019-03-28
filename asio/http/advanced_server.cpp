//------------------------------------------------------------------------------
//
// Example: Advanced server
// https://www.boost.org/doc/libs/1_69_0/libs/beast/example/advanced/server/advanced_server.cpp
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/make_unique.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;            // from <boost/beast/http.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

// 返回文件类型
boost::beast::string_view
mime_type(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == boost::beast::string_view::npos)
            return boost::beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// 把请求的文件路径转为本地路径
std::string
path_cat(
    boost::beast::string_view base,
    boost::beast::string_view path)
{
    if(base.empty())
        return path.to_string();
    std::string result = base.to_string();
#if BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// 处理http请求，并发送响应信息
template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    boost::beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send)
{
    // 请求错误
    auto const bad_request =
    [&req](boost::beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
    };

    // 文件不存在
    auto const not_found =
    [&req](boost::beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // 服务器出错
    auto const server_error =
    [&req](boost::beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + what.to_string() + "'";
        res.prepare_payload();
        return res;
    };

    // 确定是head get请求
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // 确定请求文件路径为有效路径
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // 路径拼接
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // 打开文件
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // 判断文件是否存在
    if(ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // 错误处理
    if(ec)
        return send(server_error(ec.message()));

    // 文件大小
    auto const size = body.size();

    // 响应head请求
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // 响应GET请求
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
}

//------------------------------------------------------------------------------

// 报告错误
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// 回显所有收到的WebSocket消息
class websocket_session : public std::enable_shared_from_this<websocket_session>
{
    websocket::stream<tcp::socket> ws_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::asio::steady_timer timer_;
    boost::beast::multi_buffer buffer_;
    char ping_state_ = 0;

public:
    explicit
    websocket_session(tcp::socket socket)
        : ws_(std::move(socket))
        , strand_(ws_.get_executor())
        , timer_(ws_.get_executor().context(),
            (std::chrono::steady_clock::time_point::max)())
    {
    }

    template<class Body, class Allocator>
    void
    do_accept(http::request<Body, http::basic_fields<Allocator>> req)
    {
        ws_.control_callback(
            std::bind(
                &websocket_session::on_control_callback,
                this,
                std::placeholders::_1,
                std::placeholders::_2));

        on_timer({});

        timer_.expires_after(std::chrono::seconds(15));

        ws_.async_accept(
            req,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void
    on_accept(boost::system::error_code ec)
    {
        if(ec == boost::asio::error::operation_aborted)
            return;
        if(ec)
            return fail(ec, "accept");
        do_read();
    }

    void
    on_timer(boost::system::error_code ec)
    {
        if(ec && ec != boost::asio::error::operation_aborted)
            return fail(ec, "timer");
        if(timer_.expiry() <= std::chrono::steady_clock::now())
        {
            if(ws_.is_open() && ping_state_ == 0)
            {
                ping_state_ = 1;
                timer_.expires_after(std::chrono::seconds(15));
                ws_.async_ping({},
                    boost::asio::bind_executor(
                        strand_,
                        std::bind(
                            &websocket_session::on_ping,
                            shared_from_this(),
                            std::placeholders::_1)));
            }
            else
            {
                ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
                ws_.next_layer().close(ec);
                return;
            }
        }
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
    }
    void
    activity()
    {
        ping_state_ = 0;
        timer_.expires_after(std::chrono::seconds(15));
    }
    void
    on_ping(boost::system::error_code ec)
    {
        if(ec == boost::asio::error::operation_aborted)
            return;
        if(ec)
            return fail(ec, "ping");
        if(ping_state_ == 1)
        {
            ping_state_ = 2;
        }
        else
        {
            BOOST_ASSERT(ping_state_ == 0);
        }
    }
    void
    on_control_callback(
        websocket::frame_type kind,
        boost::beast::string_view payload)
    {
        boost::ignore_unused(kind, payload);
        activity();
    }

    void
    do_read()
    {
        ws_.async_read(
            buffer_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }
    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        if(ec == boost::asio::error::operation_aborted)
            return;
        if(ec == websocket::error::closed)
            return;
        if(ec)
            fail(ec, "read");
        activity();
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &websocket_session::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }
    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        if(ec == boost::asio::error::operation_aborted)
            return;
        if(ec)
            return fail(ec, "write");
        buffer_.consume(buffer_.size());
        do_read();
    }
};

//处理http连接 
class http_session : public std::enable_shared_from_this<http_session>
{
    class queue
    {
        enum
        {
            limit = 8
        };
        struct work
        {
            virtual ~work() = default;
            virtual void operator()() = 0;
        };
        http_session& self_;
        std::vector<std::unique_ptr<work>> items_;
    public:
        explicit
        queue(http_session& self)
            : self_(self)
        {
            static_assert(limit > 0, "queue limit must be positive");
            items_.reserve(limit);
        }
        bool
        is_full() const
        {
            return items_.size() >= limit;
        }
        bool
        on_write()
        {
            BOOST_ASSERT(! items_.empty());
            auto const was_full = is_full();
            items_.erase(items_.begin());
            if(! items_.empty())
                (*items_.front())();
            return was_full;
        }
        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg)
        {
            struct work_impl : work
            {
                http_session& self_;
                http::message<isRequest, Body, Fields> msg_;
                work_impl(
                    http_session& self,
                    http::message<isRequest, Body, Fields>&& msg)
                    : self_(self)
                    , msg_(std::move(msg))
                {
                }
                void
                operator()()
                {
                    http::async_write(
                        self_.socket_,
                        msg_,
                        boost::asio::bind_executor(
                            self_.strand_,
                            std::bind(
                                &http_session::on_write,
                                self_.shared_from_this(),
                                std::placeholders::_1,
                                msg_.need_eof())));
                }
            };
            items_.push_back(
                boost::make_unique<work_impl>(self_, std::move(msg)));
            if(items_.size() == 1)
                (*items_.front())();
        }
    };

    tcp::socket socket_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::asio::steady_timer timer_;
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;
    queue queue_;

public:
    explicit
    http_session(
        tcp::socket socket,
        std::shared_ptr<std::string const> const& doc_root)
        : socket_(std::move(socket))
        , strand_(socket_.get_executor())
        , timer_(socket_.get_executor().context(),
            (std::chrono::steady_clock::time_point::max)())
        , doc_root_(doc_root)
        , queue_(*this)
    {
    }

    void
    run()
    {
        if(! strand_.running_in_this_thread())
            return boost::asio::post(
                boost::asio::bind_executor(
                    strand_,
                    std::bind(
                        &http_session::run,
                        shared_from_this())));

        on_timer({});
        do_read();
    }
    void
    do_read()
    {
        timer_.expires_after(std::chrono::seconds(15));
        req_ = {};
        http::async_read(socket_, buffer_, req_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &http_session::on_read,
                    shared_from_this(),
                    std::placeholders::_1)));
    }
    void
    on_timer(boost::system::error_code ec)
    {
        if(ec && ec != boost::asio::error::operation_aborted)
            return fail(ec, "timer");
        if(timer_.expires_at() == (std::chrono::steady_clock::time_point::min)())
            return;
        if(timer_.expiry() <= std::chrono::steady_clock::now())
        {
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close(ec);
            return;
        }
        timer_.async_wait(
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &http_session::on_timer,
                    shared_from_this(),
                    std::placeholders::_1)));
    }
    void
    on_read(boost::system::error_code ec)
    {
        if(ec == boost::asio::error::operation_aborted)
            return;
        if(ec == http::error::end_of_stream)
            return do_close();
        if(ec)
            return fail(ec, "read");
        if(websocket::is_upgrade(req_))
        {
            timer_.expires_at((std::chrono::steady_clock::time_point::min)());
            std::make_shared<websocket_session>(
                std::move(socket_))->do_accept(std::move(req_));
            return;
        }
        handle_request(*doc_root_, std::move(req_), queue_);
        if(! queue_.is_full())
            do_read();
    }
    void
    on_write(boost::system::error_code ec, bool close)
    {
        if(ec == boost::asio::error::operation_aborted)
            return;
        if(ec)
            return fail(ec, "write");
        if(close)
        {
            return do_close();
        }
        if(queue_.on_write())
        {
            do_read();
        }
    }
    void
    do_close()
    {
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);
    }
};

//------------------------------------------------------------------------------

// 监听者
class listener : public std::enable_shared_from_this<listener>
{
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    std::shared_ptr<std::string const> doc_root_;
public:
    listener(
        boost::asio::io_context& ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root)
        : acceptor_(ioc)
        , socket_(ioc)
        , doc_root_(doc_root)
    {
        boost::system::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }
    void
    run()
    {
        if(! acceptor_.is_open())
            return;
        do_accept();
    }
    void
    do_accept()
    {
        acceptor_.async_accept(
            socket_,
            std::bind(
                &listener::on_accept,
                shared_from_this(),
                std::placeholders::_1));
    }
    void
    on_accept(boost::system::error_code ec)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            std::make_shared<http_session>(
                std::move(socket_),
                doc_root_)->run();
        }
        do_accept();
    }
};

//------------------------------------------------------------------------------
// 多个线程运行同一ioc，异步调用ioc事件
// 注册一个信号处理任务，停止ioc运行
int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        std::cerr <<
            "Usage: advanced-server <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    advanced-server 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    boost::asio::io_context ioc{threads};

    // 创建并运行一个监听
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        doc_root)->run();

    // 注册信号处理函数，停止ioc
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](boost::system::error_code const&, int)
        {
            ioc.stop();
        });

    // 启动运行多个ioc线程
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    // 阻塞等待所有线程结束
    for(auto& t : v)
        t.join();

    return EXIT_SUCCESS;
}
