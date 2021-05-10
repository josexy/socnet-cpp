#include "../soc/http/include/HttpServer.h"
#include <signal.h>

using namespace soc::http;

HttpServer app;

void signal_handler(int) { app.quit(); }

int main() {
  signal(SIGINT, signal_handler);

  app.route("/", [](const auto &req, auto &resp) {
    resp.header("Content-Type", "text/html").body("hello world!").send();
  });

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
    resp.header("Content-Type", "text/html").status_code(404).body(body).send();
  });
  app.error_code(403, [&](const auto & /*req*/, auto &resp) {
    std::string body = R"(
  <html>
  <head><title>403 Forbidden</title></head>
  <body>
  <center><h1>403 Forbidden</h1></center>
  <hr><center>socnet</center>
  </body>
  </html>
  )";
    resp.header("Content-Type", "text/html").status_code(403).body(body).send();
  });

  app.start(InetAddress(5555));
  return 0;
}
