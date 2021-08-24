#include "../soc/http/include/HttpServer.h"

using namespace std;
using namespace soc::http;

class TestService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) override {
    req.getHeader().forEach(
        [](auto &k, auto &v) { cout << k << ": " << v << '\n'; });
    resp.setContentType("text/plain").setBody("hello world");
  }
  void doHead(const HttpRequest &req, HttpResponse &resp) override {
    doGet(req, resp);
  }
};

int main() {
  HttpServer server;
  server.addService<TestService>("/");
  server.start();
  return 0;
}
