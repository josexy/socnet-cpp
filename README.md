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
- 通过php-fpm解析PHP文件，实现动态web服务器
- 采用json配置文件
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
    resp.body("hello world");
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
        "enable_php": true,
        "default_page": [
            "index.php",
            "index.html"
        ]
    },
    "https": {
        "cert_file": "./ssl/cert.crt",
        "private_key_file": "./ssl/private.pem",
        "password": ""
    },
    "php-fpm": {
        "server_ip": "127.0.0.1",
        "server_port": 9000,
        "sock_path": "/run/php-fpm/php-fpm.sock"
    }
}
```

PS: 可能需要修改php-fpm配置文件中 `user` 和 `group` 为当前用户名。
