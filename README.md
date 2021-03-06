# socnet-cpp(socket of c++)
A high performance HTTP server based on linux epoll designed by C++ 20

用C++20实现的基于linux epoll高性能简单HTTP服务器，可在docker容器中运行，仅仅用于学习。

目前GitHub有很多使用C++实现的Web服务器，本人使用C++造轮子主要是从底层理解高性能Web服务器的实现以及如何分模块构建一个小型项目。

特点:
- 模块化设计，分而治之
- 基于linux epoll 边缘模式ET+非阻塞+线程池，提高服务器处理客户端连接的并发性
- 实现一个最小堆定时器，用于关闭空闲连接
- 利用状态机解析TCP数据流并转化为HTTP Request对象
- 通过OpenSSL实现HTTPS安全连接
- 支持Gzip压缩算法
- 通过php-fpm解析PHP文件，实现动态web服务器
- 采用json配置文件
- 支持sendfile和mmap （OpenSSL不支持sendfile，默认mmap）
- ...

## C++17/20特性
- 采用 `std::string_view` 避免string对象频繁拷贝和析构，加快服务器解析HTTP请求
- 采用 `std::optional` 处理返回值
- if/switch初始化
- ...

## 依赖库
- pthread
- zlib
- openssl

## Quick start
```bash
git clone https://github.com/josexy/socnet-cpp.git
cd socnet-cpp/build
cmake ..
make
./socnet
```

下面是一个非常简单的例子：
```cpp
#include "soc/http/include/HttpServer.h"

using namespace std;
using namespace soc::http;

class TestService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) override {
    req.getHeader().forEach(
        [](auto &k, auto &v) { cout << k << ": " << v << '\n'; });
    resp.setContentType("text/plain").setBody("hello world");
  }
};

int main() {
  HttpServer server;
  server.addService<TestService>("/");
  server.start();
  return 0;
}

```

## JSON配置文件
```json
{
    "server": {
        "listen_ip": "0.0.0.0",
        "listen_port": 5555,
        "idle_timeout": 2000,
        "server_hostname": "localhost",
        "enable_https": false,
        "enable_php": false,
        "enable_sendfile": false,
        "default_page": [
            "index.php",
            "index.html"
        ],
        "user_pass_file": "./pass_store/user_password",
        "authenticate_realm": "socnet@test",
        "session_lifetime": 10
    },
    "https": {
        "cert_file": "./ssl/cert.crt",
        "private_key_file": "./ssl/private.pem",
        "password": ""
    },
    "php-fpm": {
        "tcp_or_domain": true,
        "server_ip": "127.0.0.1",
        "server_port": 9000,
        "sock_path": "/run/php-fpm/php-fpm.sock"
    }
}
```

PS: 可能需要修改php-fpm配置文件中 `user` 和 `group` 为当前用户名。

## Docker 
该项目可在docker中运行：
```shell
# 构建
docker build -t web-socnet:v1 .
# 创建并运行容器
docker run -itd --rm --name socnet -p 8080:5555 web-socnet:v1
# 访问
curl 127.0.0.1:8080
# 进入容器
docker exec -it socnet /bin/bash
# 停止容器
docker stop socnet
```
