#include "../soc/http/include/HttpServer.h"
#include <signal.h>

using namespace soc::http;

HttpServer app;

void signal_handler(int) { app.quit(); }

int main() {
  signal(SIGINT, signal_handler);

  // Env::instance().set("MYSQL_HOST", "");
  // Env::instance().set("MYSQL_USER", "");
  // Env::instance().set("MYSQL_PASSWORD", "");
  // Env::instance().set("MYSQL_PORT", "");
  // Env::instance().set("MYSQL_DB", "");
  // Env::instance().set("MYSQL_TB", "");

  // Env::instance().set("CGI-BIN", "cgi-bin");
  // app.enable_cgi(true);

#ifdef SUPPORT_SSL_LIB
  app.set_https_certificate("ssl/cert.crt", "ssl/private.pem");
#endif

  assert(app.set_mounting_html_dir("/", "html"));
  assert(app.set_mounting_file_dir("/resources"));

  app.redirect("/json", "/get");
  app.set_url_auth("/json", HttpAuthType::Digest);

  app.route_regex("/echo/(.*?)_(\\d+)",
                  [&](const auto &req, auto &resp, const auto &match) {
                    resp.body(match[0] + " : " + match[1]).send();
                  });

  app.route("/post/", [&](const HttpRequest &req, HttpResponse &resp) {
    if (auto file1 = req.multipart().file("file1"); file1.has_value()) {
      auto &x = file1.value().get();
      cout << x.name << "\t" << x.file_name << "\t" << x.file_type << endl;
    }
    if (auto file2 = req.multipart().file("file2"); file2.has_value()) {
      auto &x = file2.value().get();
      cout << x.name << "\t" << x.file_name << "\t" << x.file_type << endl;
    }
    resp.body_html("html/post.html").send();
  });

  app.route("/get/", [&](const auto &req, auto &resp) {
    std::unique_ptr<libjson::JsonObject> root =
        std::make_unique<libjson::JsonObject>();
    auto header = root->add("header", new libjson::JsonObject);
    for (auto &[k, v] : req.header().items()) {
      header.second.get()->toJsonObject()->add(k, v);
    }
    auto query = root->add("query", new libjson::JsonObject);
    for (auto &[k, v] : req.query()) {
      query.second.get()->toJsonObject()->add(k, v);
    }
    resp.body_json(root.get()).send();
  });

  app.route("/login_basic", [&](const HttpRequest &req, auto &resp) {
    auto auth = req.auth<HttpBasicAuthType>();
    if (auth && auth->verify_user()) {
      resp.body("login successfully!").send();
    } else
      resp.auth(HttpAuthType::Basic).send();
  });

  app.route("/login_digest", [&](const HttpRequest &req, auto &resp) {
    auto auth = req.auth<HttpDigestAuthType>();
    if (auth && auth->verify_user()) {
      resp.body("login successfully!").send();
    } else {
      resp.auth(HttpAuthType::Digest).send();
    }
  });

  app.start(InetAddress(5555));
  return 0;
}
