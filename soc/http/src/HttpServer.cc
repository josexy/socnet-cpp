#include "../include/HttpServer.h"
#include "../../modules/php-fastcgi/include/PhpFastCgi.h"
#include <regex>

using namespace soc::http;

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

  server_->setMessageCallback(
      std::bind(&HttpServer::onMessage, this, std::placeholders::_1));

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
  char id[16], session_id[27]{0};
  EncodeUtil::genRandromStr(id);
  ::sprintf(session_id, "SESSIONID_%s", id);
  static const int interval = GET_CONFIG(int, "server", "session_lifetime");
  HttpSession *session = new HttpSession(session_id, interval);
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
  if (conn->getContext() == nullptr) {
    req = new HttpRequest(conn, this);
    conn->setContext(static_cast<void *>(req));
  } else {
    req = static_cast<HttpRequest *>(conn->getContext());
  }

  auto code = req->parseRequest();
  if (conn->isDisconnected() || conn->getContext() == nullptr) {
    if (req)
      delete req;
    return true;
  }
  HttpResponse resp(conn);
  if (code == HttpRequest::BAD_REQUEST) {
    // Connection: close
    conn->setKeepAlive(false);
    conn->setContext(nullptr);
    resp.setCode(HttpStatus::BAD_REQUEST).setHeader("Connection", "close");
    getErrorService()->service(*req, resp);
    resp.send();

    delete req;
    return true;
  } else if (code != HttpRequest::REQUEST_CONTENT_DONE)
    return false;

  req->reset();

  do {
    if (dispatchUrlPattern(*req, resp))
      break;
    if (dispatchMountDir(*req, resp))
      break;
  } while (0);

  associateRequestSession(*req, resp);

  // client/server error code
  if (resp.getCode() >= 400 && resp.getCode() < 600)
    getErrorService()->service(*req, resp);

  resp.send();

  delete req;
  conn->setContext(nullptr);
  return true;
}

HttpSession *HttpServer::associateSession(HttpRequest *req) {
  HttpSession *session = nullptr;
  // had already exist HttpSession
  if (auto x = req->getCookies().get("SESSIONID"); x.has_value()) {
    if (auto s = sessions_.get(x.value()); s.has_value()) {
      session = s.value().get();
      // The old session has been set to an invalid state, so it is
      // automatically deleted by the timer, and a null pointer object is
      // returned
      if (session && !session->isDestroy()) {
        session->setStatus(HttpSession::Accessed);
        // reset session timer
        server_->getSessionTimer()->adjust(Timer(
            EncodeUtil::murmurHash2(session->getId()),
            TimeStamp::nowSecond(session->getMaxInactiveInterval()), nullptr));
      } else {
        session = nullptr;
      }
    }
  }
  // create a new session
  if (session == nullptr) {
    session = createSession();
    server_->createSessionTimer(
        TimeStamp::second(session->getMaxInactiveInterval()));
    server_->getSessionTimer()->add(
        Timer(EncodeUtil::murmurHash2(session->getId()),
              TimeStamp::nowSecond(session->getMaxInactiveInterval()),
              std::bind(&HttpServer::handleSessionTimeout, this, session)));
  }
  return session;
}

void HttpServer::handleSessionTimeout(HttpSession *session) {
  if (!session)
    return;
  sessions_.remove(session->getId());
}

void HttpServer::associateRequestSession(const HttpRequest &req,
                                         HttpResponse &resp) {
  HttpSession *session = req.session_;
  if (session && session->isNew()) {
    HttpCookie cookie;
    cookie.add("SESSIONID", session->getId());
    cookie.setPath("/");
    resp.setCookie(cookie);
  }
}

bool HttpServer::dispatchMountDir(const HttpRequest &req, HttpResponse &resp) {
  bool status = false;
  if (mount_dir_.empty())
    return status;

  const std::string_view req_url = req.getUrl();

  mount_dir_.each([&](const auto &prefix, const auto &dir) {
    if (req_url.starts_with(prefix)) {
      if (dispatchFile(prefix, req_url, dir, req, resp))
        return status = true;
    }
    return false;
  });
  if (!status)
    resp.setCode(HttpStatus::NOT_FOUND);
  return status;
}

bool HttpServer::dispatchUrlPattern(const HttpRequest &req,
                                    HttpResponse &resp) {
  bool status = false;
  // basic url
  if (auto x = services_.get(req.getUrl()); x.has_value()) {
    x.value()->service(req, resp);
    return true;
  } else {
    // regex url-pattern
    if (!urlp_services_.empty()) {
      std::smatch m;
      std::regex rgx;

      urlp_services_.each([&](const auto &urlp, const auto &service) {
        rgx = std::regex(urlp, std::regex::extended | std::regex::ECMAScript);
        if (std::regex_search(req.getUrl(), m, rgx)) {
          std::vector<std::string> match;
          for (size_t i = 1; i < m.size(); ++i)
            match.emplace_back(m.str(i));
          service->service0(req, resp, match);
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

  bool find_status = false;
  // directory
  if (path.back() == '/') {
    // get default index page
    find_status = getIndexPageFileName(path);
    if (!find_status) {
      resp.setCode(HttpStatus::FORBIDDEN);
      return true;
    }
  }

  // file not exist
  if (!find_status && !FileUtil::exist(path))
    return false;

  static const bool enable_php = GET_CONFIG(bool, "server", "enable_php");

  // PHP processor
  if (path.ends_with(".php")) {
    if (enable_php)
      dispatchPhpProcessor(path, req, resp);
    else
      resp.setCode(HttpStatus::FORBIDDEN);
  } else {
    resp.setHeader("Content-Type", mappingMimeType(path)).setBodyFile(path);
  }
  return true;
}

void HttpServer::dispatchPhpProcessor(const std::string &path,
                                      const HttpRequest &req,
                                      HttpResponse &resp) {
  PhpFastCgi fcgi;
  static const bool tcp_or_domain =
      GET_CONFIG(bool, "php-fpm", "tcp_or_domain");

  if (tcp_or_domain) {
    static const std::string ip =
        GET_CONFIG(std::string, "php-fpm", "server_ip");
    static const int port = GET_CONFIG(int, "php-fpm", "server_port");
    if (!fcgi.connectPhpFpm(ip, port)) {
      // error
      resp.setCode(HttpStatus::INTERNAL_SERVER_ERROR);
      return;
    }
  } else {
    static const std::string path =
        GET_CONFIG(std::string, "php-fpm", "sock_path");
    if (!fcgi.connectPhpFpm(path)) {
      // error
      resp.setCode(HttpStatus::INTERNAL_SERVER_ERROR);
      return;
    }
  }

  fcgi.sendStartRequestRecord();
  fcgi.sendParams(FCGI_Params::SCRIPT_FILENAME, path);

  if (req.getAuth() != nullptr) {
    if (req.getAuth()->getAuthType() == HttpAuthType::Basic) {
      // Basic Authorization
      auto auth = dynamic_cast<HttpBasicAuth *>(req.getAuth());
      fcgi.sendParams(FCGI_Params::PHP_AUTH_USER, auth->getUsername());
      fcgi.sendParams(FCGI_Params::PHP_AUTH_PW, auth->getPassword());
    } else if (req.getAuth()->getAuthType() == HttpAuthType::Digest) {
      // Digest Authorization
      auto auth = dynamic_cast<HttpDigestAuth *>(req.getAuth());
      fcgi.sendParams(FCGI_Params::PHP_AUTH_DIGEST, auth->getDigestInfo());
    }
  }

  if (req.hasQueryString()) {
    fcgi.sendParams(FCGI_Params::QUERY_STRING, req.getQueryString());
  }

  if (req.getMethod() == HttpMethod::HEAD) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "HEAD");
    fcgi.sendEndRequestRecord();
  } else if (req.getMethod() == HttpMethod::GET) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "GET");
    fcgi.sendEndRequestRecord();
  } else if (req.getMethod() == HttpMethod::POST) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "POST");
    if (auto type = req.getHeader().get("Content-Type"); type.has_value()) {
      fcgi.sendParams(FCGI_Params::CONTENT_TYPE, type.value());
      fcgi.sendParams(FCGI_Params::CONTENT_LENGTH,
                      std::to_string(req.getPostData().size()));
    }
    fcgi.sendEndRequestRecord();
    fcgi.sendPost(req.getPostData());
  }

  Buffer buffer;
  auto ret = fcgi.readPhpFpm(&buffer);
  req.php_message_ = ret.second;

  std::string_view sv(buffer.peek(), buffer.readable());
  HttpHeader header;

  int index = header.parse(sv);
  resp.setHeader(header);
  if (auto x = header.get("Status"); x.has_value()) {
    std::string_view status = x.value();
    std::string_view codesv = status.substr(0, status.find_first_of(" "));
    int code = std::atoi(std::string(codesv.data(), codesv.size()).data());
    resp.setCode(code);
  }

  if (!ret.first && resp.getCode() != 200)
    return;

  sv.remove_prefix(index);
  resp.setBody(sv);
}

std::string HttpServer::mappingMimeType(const std::string_view &url) {
  size_t i = url.find_last_of(".");

  // The file has no extension
  if (i == std::string_view::npos) {
    return "application/octet-stream";
  }

  std::string type;
  if (auto it = Content_Type.find(hashExt(url.substr(i + 1)));
      it != Content_Type.end())
    type = it->second;
  else // default mime type
    type = "application/octet-stream";

  if (type.starts_with("text") || type.ends_with("xml") ||
      type.ends_with("javascript") || type.ends_with("json")) {
    type += " ;charset=utf-8";
  }
  return type;
}

bool HttpServer::getIndexPageFileName(std::string &dir) {
  for (const auto &index_page : default_pages_) {
    if (FileUtil::exist(dir + index_page)) {
      dir += index_page;
      return true;
    }
  }
  return false;
}