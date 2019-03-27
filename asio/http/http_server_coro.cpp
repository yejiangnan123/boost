//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//
// 基于协程，协程开发方便，效率高，协程类似线程，但是没有线程消耗，调用异步操作之后自动挂起，异步完成后继续执行
//------------------------------------------------------------------------------
//
// Example: HTTP server, coroutine
// https://www.boost.org/doc/libs/1_69_0/libs/beast/example/http/server/coro/http_server_coro.cpp
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
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

//此函数为给定的事件生成HTTP响应请求 
//响应对象的类型取决于请求的内容，所以接口需要调用者传递一个通用lambda来接收响应。
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

// 这是与通用lambda相当的C ++ 11。
// 函数对象用于发送HTTP消息。
template<class Stream>
struct send_lambda
{
    Stream& stream_;
    bool& close_;
    boost::system::error_code& ec_;
    boost::asio::yield_context yield_;

    explicit
    send_lambda(
        Stream& stream,
        bool& close,
        boost::system::error_code& ec,
        boost::asio::yield_context yield)
        : stream_(stream)
        , close_(close)
        , ec_(ec)
        , yield_(yield)
    {
    }

    template<bool isRequest, class Body, class Fields>
    void
    operator()(http::message<isRequest, Body, Fields>&& msg) const
    {
        // 确定我们是否应该关闭连接
        close_ = msg.need_eof();

        //我们需要序列化器，因为序列化器需要
    //一个非const file_body，以及面向消息的版本
    // http :: write仅适用于const消息。
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::async_write(stream_, sr, yield_[ec_]);
    }
};

// 处理HTTP服务器连接
void
do_session(
    tcp::socket& socket,
    std::shared_ptr<std::string const> const& doc_root,
    boost::asio::yield_context yield)
{
    bool close = false;
    boost::system::error_code ec;

    // 此缓冲区需要在读取期间保持不变
    boost::beast::flat_buffer buffer;

    // 这个lambda用于发送消息
    send_lambda<tcp::socket> lambda{socket, close, ec, yield};

    for(;;)
    {
        // 阅读请求
        http::request<http::string_body> req;
        http::async_read(socket, buffer, req, yield[ec]);
        if(ec == http::error::end_of_stream)
            break;
        if(ec)
            return fail(ec, "read");

        // 发送回复
        handle_request(*doc_root, std::move(req), lambda);
        if(ec)
            return fail(ec, "write");
        if(close)
        {
            //这意味着我们应该关闭连接，通常是因为
      //响应表示“连接：关闭”语义。
            break;
        }
    }

    // 发送TCP关闭
    socket.shutdown(tcp::socket::shutdown_send, ec);

    // 此时连接正常关闭
}

//------------------------------------------------------------------------------

// 接受传入的连接并启动会话
void
do_listen(
    boost::asio::io_context& ioc,
    tcp::endpoint endpoint,
    std::shared_ptr<std::string const> const& doc_root,
    boost::asio::yield_context yield)
{
    boost::system::error_code ec;

    // 打开接受者
    tcp::acceptor acceptor(ioc);
    acceptor.open(endpoint.protocol(), ec);
    if(ec)
        return fail(ec, "open");

    // 允许地址重用
    acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if(ec)
        return fail(ec, "set_option");

    // 绑定到服务器地址
    acceptor.bind(endpoint, ec);
    if(ec)
        return fail(ec, "bind");

    // 开始侦听连接
    acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if(ec)
        return fail(ec, "listen");

    for(;;)
    {
        tcp::socket socket(ioc);
        acceptor.async_accept(socket, yield[ec]);
        if(ec)
            fail(ec, "accept");
        else
            boost::asio::spawn(
                acceptor.get_executor().context(),
                std::bind(
                    &do_session,
                    std::move(socket),
                    doc_root,
                    std::placeholders::_1));
    }
}

int main(int argc, char* argv[])
{
    // 检查命令行参数。
    if (argc != 5)
    {
        std::cerr <<
            "Usage: http-server-coro <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-coro 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // 所有I / O都需要io_context
    boost::asio::io_context ioc{threads};

    // 产生一个侦听端口
    boost::asio::spawn(ioc,
        std::bind(
            &do_listen,
            std::ref(ioc),
            tcp::endpoint{address, port},
            doc_root,
            std::placeholders::_1));

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
