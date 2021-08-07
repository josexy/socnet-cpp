#include "../include/HttpServer.h"
#include "../../modules/php-fastcgi/include/PhpFastCgi.h"
#include <regex>

using namespace soc::http;
using std::placeholders::_1;

void HttpServer::DefaultErrorService::doError(int code, const HttpRequest &req,
                                              HttpResponse &resp) {
  resp.header("Content-Type", "text/html").body(baseErrorPage(code));
}

std::string HttpServer::DefaultErrorService::baseErrorPage(int code) {
  std::string text = std::to_string(code) + " " + Status_Code.at(code);
  return R"(<html><head><title>)" + text +
         R"(</title></head><body><center><h1>)" + text +
         R"(</h1></center><hr><center>socnet</center></body></html>)";
}

HttpServer::HttpServer() {
  server_ = std::make_unique<TcpServer>();
  initialize();
}

HttpServer::~HttpServer() {
  sessions_.clear();
  services_.clear();
  urlp_services_.clear();
}

void HttpServer::initialize() {
  ::srand(::time(nullptr));
  setIdleTime(GET_CONFIG(int, "server", "idle_timeout"));

  default_pages_ =
      GET_CONFIG(std::vector<std::string>, "server", "default_page");

  if (GET_CONFIG(bool, "server", "enable_https")) {
    setCertificate(GET_CONFIG(std::string, "https", "cert_file"),
                   GET_CONFIG(std::string, "https", "private_key_file"),
                   GET_CONFIG(std::string, "https", "password"));
  }

  server_->setMessageCallback(std::bind(&HttpServer::onMessage, this, _1));

  setErrorService<DefaultErrorService>();
}

BaseService *HttpServer::getErrorService() const {
  return services_.get("#").value().get();
}

void HttpServer::start() {
  InetAddress address(GET_CONFIG(std::string, "server", "listen_ip"),
                      GET_CONFIG(int, "server", "listen_port"));
  server_->start(address);
}

void HttpServer::setIdleTime(int millsecond) {
  if (millsecond <= 0)
    millsecond = 2000;
  server_->setIdleTime(millsecond);
}

void HttpServer::setCertificate(const std::string &cert_file,
                                const std::string &private_key_file,
                                const std::string &password) {
  server_->setCertificate(cert_file, private_key_file, password);
}

HttpSession *HttpServer::createSession() {
  std::string session_id = "SESSIONID_" + EncodeUtil::genRandromStr(16);
  HttpSession *session = new HttpSession(session_id);
  sessions_.add(session_id, std::shared_ptr<HttpSession>(session));
  return session;
}

void HttpServer::quit() { server_->quit(); }

void HttpServer::addMountDir(const std::string &url, const std::string &dir) {
  std::string u = url;
  if (u.back() != '/')
    u += "/";
  mount_dir_.add(u, std::filesystem::canonical(dir).string() + "/");
}

bool HttpServer::onMessage(TcpConnection *conn) {
  HttpRequest *req = nullptr;
  if (conn->context() == nullptr) {
    req = new HttpRequest(conn->recver(), this);
    conn->setContext(static_cast<void *>(req));
  } else {
    req = static_cast<HttpRequest *>(conn->context());
  }

  auto code = req->parseRequest();
  if (conn->disconnected() || conn->context() == nullptr) {
    if (req)
      delete req;
    return true;
  }
  HttpResponse resp(conn);
  if (code == HttpRequest::BAD_REQUEST) {
    // Connection: close
    conn->setKeepAlive(false);
    conn->setContext(nullptr);
    resp.code(400).header("Connection", "close");
    getErrorService()->service(*req, resp);
    resp.send();

    delete req;
    return true;
  } else if (code != HttpRequest::REQUEST_CONTENT_DONE)
    return false;

  req->initialize();

  do {
    if (dispatchUrlPattern(*req, resp))
      break;
    if (dispatchMountDir(*req, resp))
      break;

    resp.code(404);
    getErrorService()->service(*req, resp);

  } while (0);

  associateRequestSession(*req, resp);

  resp.send();

  delete req;
  conn->setContext(nullptr);
  return true;
}

HttpSession *HttpServer::associateSession(HttpRequest *req) {
  HttpSession *session = nullptr;
  // had already exist HttpSession
  if (auto x = req->cookies().get("SESSIONID"); x.has_value()) {
    if (auto s = sessions_.get(x.value()); s.has_value()) {
      session = s.value().get();
      // The old session has been set to an invalid state, so it is
      // automatically deleted by the timer, and a null pointer object is
      // returned
      if (session && !session->isDestroy()) {
        session->setStatus(HttpSession::Accessed);
        // reset session timer
        server_->getSessionTimer()->adjust(Timer(
            EncodeUtil::murmurHash2(session->id()),
            TimeStamp::nowSecond(session->maxInactiveInterval()), nullptr));
      } else {
        session = nullptr;
      }
    }
  }
  // create a new session
  if (session == nullptr) {
    session = createSession();
    server_->createSessionTimer(
        TimeStamp::second(session->maxInactiveInterval()));
    server_->getSessionTimer()->add(
        Timer(EncodeUtil::murmurHash2(session->id()),
              TimeStamp::nowSecond(session->maxInactiveInterval()),
              std::bind(&HttpServer::handleSessionTimeout, this, session)));
  }
  return session;
}

void HttpServer::handleSessionTimeout(HttpSession *session) {
  if (!session)
    return;
  sessions_.remove(session->id());
}

void HttpServer::associateRequestSession(const HttpRequest &req,
                                         HttpResponse &resp) {
  HttpSession *session = req.session_;
  if (session && session->isNew()) {
    HttpCookie cookie;
    cookie.add("SESSIONID", session->id());
    cookie.setPath("/");
    resp.cookie(cookie);
  }
}

bool HttpServer::dispatchMountDir(const HttpRequest &req, HttpResponse &resp) {
  bool status = false;
  if (mount_dir_.empty())
    return status;

  const std::string_view req_url = req.url();

  mount_dir_.each([&](const auto &prefix, const auto &dir) {
    if (req_url.starts_with(prefix)) {
      if (dispatchFile(prefix, req_url, dir, req, resp))
        return status = true;
    }
    return false;
  });

  return status;
}

bool HttpServer::dispatchUrlPattern(const HttpRequest &req,
                                    HttpResponse &resp) {
  bool status = false;
  // basic url
  if (auto x = services_.get(req.url()); x.has_value()) {
    x.value()->service(req, resp);
    return true;
  } else {
    // regex url-pattern
    if (!urlp_services_.empty()) {
      std::smatch m;
      std::regex rgx;

      urlp_services_.each([&](const auto &urlp, const auto &service) {
        rgx = std::regex(urlp, std::regex::extended | std::regex::ECMAScript);
        if (std::regex_search(req.url(), m, rgx)) {
          std::vector<std::string> match;
          for (size_t i = 1; i < m.size(); ++i)
            match.emplace_back(m.str(i));
          service->service(req, resp, match);
          return status = true;
        }
        return false;
      });
    }
  }
  return status;
}

bool HttpServer::dispatchFile(std::string_view prefix, std::string_view req_url,
                              std::string path, const HttpRequest &req,
                              HttpResponse &resp) {
  // suffix
  req_url.remove_prefix(prefix.size());
  // absolute path
  path.append(req_url.data(), req_url.size());

  bool enable_php = GET_CONFIG(bool, "server", "enable_php");

  // directory
  if (path.back() == '/') {
    // get default index page
    getIndexPageFileName(enable_php, path);
  }

  // page not found
  if (!FileUtil::exist(path))
    return false;

  // PHP processor
  if (path.ends_with(".php")) {
    if (!enable_php) {
      resp.code(404);
      getErrorService()->service(req, resp);
    } else if (!dispatchPhpProcessor(path, req, resp)) {
      getErrorService()->service(req, resp);
    }
  } else {
    resp.header("Content-Type", mappingMimeType(path)).bodyFile(path);
  }
  return true;
}

bool HttpServer::dispatchPhpProcessor(const std::string &path,
                                      const HttpRequest &req,
                                      HttpResponse &resp) {
  PhpFastCgi fcgi;
  if (EXIST_CONFIG("php-fpm", "server_ip")) {
    if (!fcgi.connectPhpFpm(GET_CONFIG(std::string, "php-fpm", "server_ip"),
                            GET_CONFIG(int, "php-fpm", "server_port"))) {
      // error
      resp.code(500);
      return false;
    }
  } else if (EXIST_CONFIG("php-fpm", "sock_path")) {
    if (!fcgi.connectPhpFpm(GET_CONFIG(std::string, "php-fpm", "sock_path"))) {
      // error
      resp.code(500);
      return false;
    }
  } else {
    // error
    resp.code(500);
    return false;
  }

  fcgi.sendStartRequestRecord();
  fcgi.sendParams(FCGI_Params::SCRIPT_FILENAME, path);

  if (req.auth() != nullptr) {
    if (req.auth()->type() == HttpAuthType::Basic) {
      // Basic Authorization
      auto auth = dynamic_cast<HttpBasicAuth *>(req.auth());
      fcgi.sendParams(FCGI_Params::PHP_AUTH_USER, auth->username());
      fcgi.sendParams(FCGI_Params::PHP_AUTH_PW, auth->password());
    } else if (req.auth()->type() == HttpAuthType::Digest) {
      // Digest Authorization
      auto auth = dynamic_cast<HttpDigestAuth *>(req.auth());
      fcgi.sendParams(FCGI_Params::PHP_AUTH_DIGEST, auth->digestInfo());
    }
  }

  if (req.hasQueryString()) {
    fcgi.sendParams(FCGI_Params::QUERY_STRING, req.queryStr());
  }

  if (req.method() == HttpMethod::HEAD) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "HEAD");
    fcgi.sendEndRequestRecord();
  } else if (req.method() == HttpMethod::GET) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "GET");
    fcgi.sendEndRequestRecord();
  } else if (req.method() == HttpMethod::POST) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "POST");
    if (auto type = req.header().get("Content-Type"); type.has_value()) {
      fcgi.sendParams(FCGI_Params::CONTENT_TYPE, type.value());
      fcgi.sendParams(FCGI_Params::CONTENT_LENGTH,
                      std::to_string(req.postData().size()));
    }
    fcgi.sendEndRequestRecord();
    fcgi.sendPost(req.postData());
  }

  Buffer buffer;
  auto ret = fcgi.readPhpFpm(&buffer);
  req.php_message_ = ret.second;

  std::string_view sv(buffer.peek(), buffer.readable());
  HttpHeader header;

  int index = header.parse(sv);
  resp.header(header);
  if (auto x = header.get("Status"); x.has_value()) {
    std::string_view status = x.value();
    std::string_view codesv = status.substr(0, status.find_first_of(" "));
    int code = std::atoi(std::string(codesv.data(), codesv.size()).data());
    resp.code(code);
  }

  if (!ret.first && resp.code() != 200)
    return false;

  sv.remove_prefix(index);
  resp.body(sv);
  return true;
}

std::string HttpServer::mappingMimeType(const std::string_view &url) {
  size_t i = url.find_last_of(".");

  std::string type;
  if (i == std::string_view::npos) {
    return type = "*/*";
  }
  if (auto it = Content_Type.find(hash_ext(url.substr(i + 1)));
      it != Content_Type.end())
    type = it->second;
  else
    type = "*/*"; // or text/plain

  if (!type.starts_with("*/*") &&
      (type.starts_with("text") || type.ends_with("xml") ||
       type.ends_with("javascript") || type.ends_with("json"))) {
    type += " ;charset=utf-8";
  }
  return type;
}

void HttpServer::getIndexPageFileName(bool php, std::string &dir) {
  for (const auto &index_page : default_pages_) {
    if (!php && index_page.ends_with(".php"))
      continue;
    if (FileUtil::exist(dir + index_page)) {
      dir += index_page;
      break;
    }
  }
}