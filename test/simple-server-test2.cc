#include "../soc/http/include/HttpServer.h"

using namespace std;
using namespace soc::http;

class LoginTestService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) override {
    if (req.getAuth() && req.getAuth()->verify()) {
      HttpCookie cookie;
      cookie.add("id", "login");
      cookie.setMaxAge(10);
      cookie.setPath("/");

      resp.setCookie(cookie)
          .setContentType("text/plain")
          .setBody("login successfully!");
    } else
      resp.sendAuth(HttpAuthType::Basic);
  }

  void doPost(const HttpRequest &req, HttpResponse &resp) override {
    doGet(req, resp);
  }
};

class PostTestService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) override {
    HttpSession *session = req.getSession();
    if (req.getCookies().get("user").has_value()) {
      resp.setContentType("text/plain").setBody("post test successfully!");
    } else {
      resp.setBodyHtml("html/post.html");
    }
  }

  void doPost(const HttpRequest &req, HttpResponse &resp) override {
    // Content-Type: multipart/form-data
    if (req.hasMultiPart()) {
      auto &multipart = req.getMultiPart();

      std::cout << "boundary: " << multipart.getBoundary() << '\n';
      if (auto x = multipart.getValue("address"); x.has_value())
        cout << x.value() << '\n';

      if (auto x = multipart.getValue("phone"); x.has_value())
        cout << x.value() << '\n';

      if (auto file1 = multipart.getFile("file1"); file1 != nullptr) {
        cout << "file1: " << file1->name << '\t' << file1->file_type << '\t'
             << file1->file_name << '\n';
      }

      if (auto file2 = multipart.getFile("file2"); file2 != nullptr) {
        cout << "file2: " << file2->name << '\t' << file2->file_type << '\t'
             << file2->file_name << '\n';
      }

      resp.setContentType("text/plain").setBody("post test successfully!");
      return;
    }

    // Content-Type: application/x-www-form-urlencoded
    if (req.hasForm()) {
      std::string name = req.getForm().get("name").value();
      std::string age = req.getForm().get("age").value();
      cout << "name: " << name << '\n' << "age: " << age << '\n';
      if (name == "admin") {
        HttpCookie cookie;
        cookie.setMaxAge(5);
        cookie.add("user", name);
        resp.setCookie(cookie)
            .setContentType("text/plain")
            .setBody("post test successfully!");
        return;
      }
    }
    resp.setBodyHtml("html/post.html");
  }
};

class ErrorService : public HttpErrorService {
public:
  bool doError(int code, const HttpRequest &req, HttpResponse &resp) override {
    if (code == 404) {
      resp.setBodyHtml("html/error/404.html");
      return true;
    } else if (code == 500) {
      resp.setBodyHtml("html/error/500.html");
      return true;
    }
    return false;
  }
};

class SetSessionService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) {
    HttpSession *session = req.getSession();
    cout << "session id: " << session->getId() << endl;
    session->setValue("str", std::string("hello world"));
    session->setValue("num", 1000);

    // session->invalidate();
    resp.setContentType("text/plain").setBody("set session");
  }
};

class GetSessionService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) {
    HttpSession *session = req.getSession();
    cout << "session id: " << session->getId() << endl;
    if (auto x = session->getValue<std::string>("str"); x) {
      cout << "str: " << *x << endl;
    }
    if (auto x = session->getValue<int>("num"); x) {
      cout << "num: " << *x << endl;
    }
    resp.setContentType("text/plain").setBody("get session");
  }
};

class GetHeaderService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) {
    auto root = std::make_shared<JsonObject>();
    auto e1 = root->add("headers", new JsonObject).second->toJsonObject();
    req.getHeader().forEach(
        [&](const auto &k, const auto &v) { e1->add(k, v); });
    auto e2 = root->add("others", new JsonObject).second->toJsonObject();
    e2->add("full_url", req.getFullUrl());
    e2->add("url", req.getUrl());
    e2->add("query_string", req.getQueryString());
    e2->add("remote_address", req.getInetAddress().toString());
    resp.setBodyJson(root.get());
  }
};

HttpServer server;

void handlerSignal(int) { server.quit(); }

int main() {
  ::signal(SIGINT, handlerSignal);

  server.addMountDir("/", "./html");
  server.addService<SetSessionService>("/set");
  server.addService<GetSessionService>("/get");
  server.addService<GetHeaderService>("/json");
  server.addService<LoginTestService>("/login");
  server.addService<PostTestService>("/post");

  // server.addUrlPatternService<PostTestService>("/regex/(.*?)$");

  server.setErrorService<ErrorService>();
  server.start();
  return 0;
}
