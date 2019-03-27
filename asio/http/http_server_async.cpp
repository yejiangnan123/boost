//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, asynchronous
// https://www.boost.org/doc/libs/1_69_0/libs/beast/example/http/server/async/http_server_async.cpp
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

// 根据文件的扩展名返回合理的mime类型。
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

// 将HTTP rel-path附加到本地文件系统路径。
// 返回的路径针对平台进行规范化。
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

// 此函数为给定的事件生成HTTP响应请求。 
// 响应对象的类型取决于请求的内容，所以接口需要调用者传递一个通用的lambda来接收响应。
template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    boost::beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send)
{
    // 返回错误的请求响应
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

    // 返回未找到的响应
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

    // 返回服务器错误响应
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

    // 确保我们可以处理该方法
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return send(bad_request("Unknown HTTP-method"));

    // 请求路径必须是绝对的，不包含“..”。
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    // 构建所请求文件的路径
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // 尝试打开文件
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // 处理文件不存在的情况
    if(ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(req.target()));

    // 处理未知错误
    if(ec)
        return send(server_error(ec.message()));

    // 移动后我们需要它来缓存大小
    auto const size = body.size();

    // 回应HEAD请求
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return send(std::move(res));
    }

    // 回应GET请求
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

// 报告失败
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// 处理HTTP服务器连接
class session : public std::enable_shared_from_this<session>
{
    // 这是与通用lambda相当的C ++ 11。
    // 函数对象用于发送HTTP消息。
    struct send_lambda
    {
        session& self_;

        explicit
        send_lambda(session& self)
            : self_(self)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
        operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // 消息的生命周期必须延长
            // 因为异步操作的持续时间
            // 我们使用shared_ptr来管理它。
            auto sp = std::make_shared<
                http::message<isRequest, Body, Fields>>(std::move(msg));

            // 存储共享的类型擦除版本
            // 类中的指针使其保持活动状态。
            self_.res_ = sp;

            // 写下回复
            http::async_write(
                self_.socket_,
                *sp,
                boost::asio::bind_executor(
                    self_.strand_,
                    std::bind(
                        &session::on_write,
                        self_.shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2,
                        sp->need_eof())));
        }
    };

    tcp::socket socket_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

public:
    // 获得套接字的所有权
    explicit
    session(
        tcp::socket socket,
        std::shared_ptr<std::string const> const& doc_root)
        : socket_(std::move(socket))
        , strand_(socket_.get_executor())
        , doc_root_(doc_root)
        , lambda_(*this)
    {
    }

    // 启动异步操作
    void
    run()
    {
        do_read();
    }

    void
    do_read()
    {
        // 在阅读之前将请求清空
        // 否则操作行为未定义。
        req_ = {};

        // Read a request
        http::async_read(socket_, buffer_, req_,
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &session::on_read,
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

        // 这意味着他们关闭了连接
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // 发送回复
        handle_request(*doc_root_, std::move(req_), lambda_);
    }

    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred,
        bool close)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(close)
        {
            // 这意味着我们应该关闭连接，通常是因为响应表明“连接：关闭”语义。
            return do_close();
        }

        // 我们完成了响应，所以删除它
        res_ = nullptr;

        // 阅读其他请求
        do_read();
    }

    void
    do_close()
    {
        // 发送TCP关闭
        boost::system::error_code ec;
        socket_.shutdown(tcp::socket::shutdown_send, ec);

        // 此时连接正常关闭
    }
};

//------------------------------------------------------------------------------

// 接受传入的连接并启动会话
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

        // 打开接受者
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // 允许地址重用
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // 绑定到服务器地址
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // 开始侦听连接
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // 开始接受传入连接
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
            // 创建会话并运行它
            std::make_shared<session>(
                std::move(socket_),
                doc_root_)->run();
        }

        // 接受另一个连接
        do_accept();
    }
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // 检查命令行参数。
    if (argc != 5)
    {
        std::cerr <<
            "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-async 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // 所有I / O都需要io_context
    boost::asio::io_context ioc{threads};

    // 创建并启动侦听端口
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        doc_root)->run();

    // 在请求的线程数上运行I / O服务
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}
