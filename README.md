# socnet-cpp(socket of c++)
A high performance HTTP server based on linux epoll designed by C++ 20

用C++20实现的基于linux epoll高性能简单HTTP服务器，用于学习目的。

特点:
- 模块化设计，分而治之
- 基于linux epoll 边缘模式ET+非阻塞+线程池，提高服务器处理客户端连接的并发性
- 实现一个最小堆定时器，用于关闭空闲连接
- 利用状态机解析TCP数据流并转化为HTTP Request对象
- 通过OpenSSL实现HTTPS安全连接
- 支持Gzip压缩算法
- ...

## C++17/20特性
- 采用 `std::string_view` 避免string对象频繁拷贝和析构，加快服务器解析HTTP请求
- 采用 `std::optional` 处理返回值
- if/switch初始化
- ...

## Quick start
```cpp
#include "soc/http/include/HttpServer.h"
#include <signal.h>

using namespace soc::http;

HttpServer app;

void signal_handler(int) { app.quit(); }

int main() {
  // 捕获中断信号，安全退出
  signal(SIGINT, signal_handler);

  app.route("/", [](const auto &req, auto &resp) {
    resp.body("hello world!").send();
  });

  app.start(InetAddress(5555));
  return 0;
}
```

## HTTPS安全传输
### OpenSSL 生成证书
```bash
openssl genrsa -out private.pem 2048
openssl req -new -x509 -key private.pem -out cert.crt -days 99999
```

开启宏 `SUPPORT_SSL_LIB`

```cpp
#include "soc/http/include/HttpServer.h"
#include <signal.h>

using namespace soc::http;

HttpServer app;

void signal_handler(int) { app.quit(); }

int main() {
  signal(SIGINT, signal_handler);

  app.set_https_certificate("ssl/cert.crt", "ssl/private.pem");

  assert(app.set_mounting_html_dir("/", "html"));
  app.start(InetAddress(5555));
  return 0;
}

```

## HttpServer 基本API
### route
通过 `route()` 可对请求url进行分发，即传入一个callback可获取客户端请求并对其进行响应

需要注意的是对于后缀是`/`的url，比如 `/get/`，那么 访问 `/get/` 或者 `/get` 均可；如果是 `/get` ，那么只能访问 `/get` 。这参考了 python flask

```cpp
app.route("/", [](const HttpRequest &req, HttpResponse &resp) {
    resp.body("hello world!").send();
});
```
### route_regex
`route_regex()` 类似 `route()`，不过该接口通过正则表达式(`std::regex`)对请求url进行匹配过滤并返回捕获的分组

比如客户端请求的url: http://127.0.0.1:5555/echo/get_100 返回分组 `match[0]="get"` 和 `match[1]="100"`
```cpp
app.route_regex("/echo/(.*?)_(\\d+)",[&](const auto &req, auto &resp, const auto &match) {
  resp.body(match[0] + " : " + match[1]).send();
});
```

### error_code
该接口可对HTTP状态码重定向，比如常见的404错误。（注意：该接口是否能够对某些状态码进行重定向依赖于HttpServer内部的实现逻辑）
```cpp
app.error_code(404, [&](const auto & /*req*/, auto &resp) {
  std::string body = R"(
  <html>
  <head><title>404 Not Found</title></head>
  <body>
  <center><h1>404 Not Found</h1></center>
  <hr><center>socnet</center>
  </body>
  </html>
  )";
  // 默认返回状态码: 200
  resp.header("Content-Type", "text/html").status_code(404).body(body).send();
  });
```

### redirect
该接口对某个 url 进行重定向，简单支持多重重定向

```cpp
// /json2 -> /json -> /get
app.route("/get",...);
app.redirect("/json", "/get");
app.redirect("/json2", "/json");
```

### set_url_auth
开启后客户端在访问url时必须进行身份验证，仅支持Basic和Digest两种身份验证。

```cpp
app.set_url_auth("/", HttpAuthType::Basic);
app.set_url_auth("/json", HttpAuthType::Digest);
```

除此之外，该HTTP服务器还支持本地和远程(MySQL)存储验证的帐号和密码，密码保存md5哈希值，格式为： `md5(username:realm:password)`。

默认本地文件存储 `pass_store/user_password`

命令行生成：
```bash
echo -n "admin:socnet@test:admin"|md5sum
echo -n "guest:socnet@test:guest"|md5sum
echo -n "user1:socnet@test:12345"|md5sum
```
