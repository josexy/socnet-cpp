#include "../soc/http/include/HttpServer.h"

using namespace std;
using namespace soc::http;

class LoginTestService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) override {
    req.header().forEach([](auto k, auto v) { cout << k << ":" << v << '\n'; });

    HttpCookie cookie;
    cookie.add("id", "login");
    cookie.setMaxAge(10);
    cookie.setPath("/");

    if (req.auth() && req.auth()->verify()) {
      resp.cookie(cookie).body("login successfully!");
    } else {
      resp.sendAuth(HttpAuthType::Basic);
      return;
    }
  }

  void doPost(const HttpRequest &req, HttpResponse &resp) override {
    doGet(req, resp);
  }
};

class PostTestService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) override {
    HttpSession *session = req.session();
    if (req.cookies().get("user").has_value()) {
      resp.body("post test successfully!");
    } else {
      resp.bodyHtml("html/post.html");
    }
  }

  void doPost(const HttpRequest &req, HttpResponse &resp) override {
    // Content-Type: multipart/form-data
    if (req.hasMultiPart()) {
      auto &multipart = req.multiPart();

      std::cout << "boundary: " << multipart.boundary() << '\n';
      if (auto x = multipart.value("address"); x.has_value())
        cout << x.value() << '\n';

      if (auto x = multipart.value("phone"); x.has_value())
        cout << x.value() << '\n';

      if (auto file1 = multipart.file("file1"); file1 != nullptr) {
        cout << "file1: " << file1->name << '\t' << file1->file_type << '\t'
             << file1->file_name << '\n';
      }

      if (auto file2 = multipart.file("file2"); file2 != nullptr) {
        cout << "file2: " << file2->name << '\t' << file2->file_type << '\t'
             << file2->file_name << '\n';
      }

      resp.body("post test successfully!");
      return;
    }

    // Content-Type: application/x-www-form-urlencoded
    if (req.hasForm()) {
      std::string name = req.form().get("name").value();
      if (name == "admin") {
        HttpCookie cookie;
        cookie.setMaxAge(5);
        cookie.add("user", name);
        resp.cookie(cookie).body("post test successfully!");
        return;
      }
    }
    resp.bodyHtml("html/post.html");
  }
};

class ErrorService : public HttpErrorService {
public:
  void doError(int code, const HttpRequest &req, HttpResponse &resp) override {
    if (code == 404) {
      resp.bodyHtml("html/error/404.html");
    } else if (code == 500) {
      resp.bodyHtml("html/error/500.html");
    }
  }
};

class SetSessionService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) {
    HttpSession *session = req.session();
    cout << "session id: " << session->id() << endl;
    session->setValue("str", std::string("hello world"));
    session->setValue("num", 1000);

    // session->invalidate();
    resp.body("set session");
  }
};

class GetSessionService : public HttpService {
public:
  void doGet(const HttpRequest &req, HttpResponse &resp) {
    HttpSession *session = req.session();
    cout << "session id: " << session->id() << endl;
    if (auto x = session->getValue<std::string>("str"); x) {
      cout << "str: " << *x << endl;
    }
    if (auto x = session->getValue<int>("num"); x) {
      cout << "num: " << *x << endl;
    }

    resp.body("get session");
  }
};

HttpServer server;

void handlerSignal(int) { server.quit(); }

int main() {
  ::signal(SIGINT, handlerSignal);

  server.addMountDir("/", ".");

  server.addService<SetSessionService>("/set");
  server.addService<GetSessionService>("/get");

  server.addService<LoginTestService>("/login");
  server.addService<PostTestService>("/post");

  server.addUrlPatternService<PostTestService>("/regex/(.*?)$");

  server.setErrorService<ErrorService>();
  server.start();
  return 0;
}
