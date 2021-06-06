#include "../soc/http/include/HttpServer.h"
#include <signal.h>

using namespace soc::http;
HttpServer app;

void signal_handler(int) { app.quit(); }

int main() {
  signal(SIGINT, signal_handler);

  app.set_mounting_html_dir("/", ".");

  app.redirect("/json", "/get");
  app.set_url_auth("/json", HttpAuthType::Digest);

  app.route_regex("/echo/(.*?)_(\\d+)",
                  [&](const auto &req, auto &resp, const auto &match) {
                    resp.body(match[0] + " : " + match[1]).send();
                  });

  app.route("/post/", [&](const HttpRequest &req, HttpResponse &resp) {
    if (auto file1 = req.multipart().file("file1"); file1.has_value()) {
      auto &x = file1.value().get().at(0);
      cout << x.name << "\t" << x.file_name << "\t" << x.file_type << endl;
    }
    if (auto file2 = req.multipart().file("file2"); file2.has_value()) {
      auto &x = file2.value().get().at(0);
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
    if (GET_CONFIG(bool, "server", "enable_mysql")) {
      auto auth = req.auth<HttpBasicSqlAuth>();
      if (auth && auth->verify_user()) {
        resp.body("login successfully!").send();
        return;
      }
    } else {
      auto auth = req.auth<HttpBasicLocalAuth>();
      if (auth && auth->verify_user()) {
        resp.body("login successfully!").send();
        return;
      }
    }
    resp.auth(HttpAuthType::Basic).send();
  });

  app.route("/login_digest", [&](const HttpRequest &req, auto &resp) {
    if (GET_CONFIG(bool, "server", "enable_mysql")) {
      auto auth = req.auth<HttpDigestSqlAuth>();
      if (auth && auth->verify_user()) {
        resp.body("login successfully!").send();
        return;
      }
    } else {
      auto auth = req.auth<HttpDigestLocalAuth>();
      if (auth && auth->verify_user()) {
        resp.body("login successfully!").send();
        return;
      }
    }
    resp.auth(HttpAuthType::Digest).send();
  });

  app.start();
  return 0;
}
