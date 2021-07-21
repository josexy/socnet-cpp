#include "../soc/http/include/HttpServer.h"

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
