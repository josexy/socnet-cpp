
#include "../include/HttpServer.h"
#include "../../modules/cgi/include/CGI.h"
#include "../../modules/mysql/include/MYSQL.h"
#include "../../modules/php-fastcgi/include/PhpFastCgi.h"
#include "../../utility/include/FileLister.h"
#include "../../utility/include/Logger.h"
#include <filesystem>
#include <regex>
using namespace soc::http;
using std::placeholders::_1;

HttpServer::HttpServer() {
  server_ = std::make_unique<TcpServer>();
  initialize();
}

HttpServer::~HttpServer() {}

void HttpServer::initialize() {
  ::srand(::time(nullptr));
  set_idle_timeout(GET_CONFIG(int, "server", "idle_timeout"));
  enable_cgi(GET_CONFIG(bool, "server", "enable_simple_cgi"));

  default_page_ =
      GET_CONFIG(std::vector<std::string>, "server", "default_page");

  if (GET_CONFIG(bool, "server", "enable_https")) {
    set_https_certificate(GET_CONFIG(std::string, "https", "cert_file"),
                          GET_CONFIG(std::string, "https", "private_key_file"),
                          GET_CONFIG(std::string, "https", "password"));
  }
  server_->set_msg_cb(std::bind(&HttpServer::handle_msg, this, _1));

  code_handlers_.emplace(404, [](const auto & /*req*/, auto &resp) {
    resp.status_code(404).body("404 not found!").send();
  });
  code_handlers_.emplace(500, [](const auto & /*req*/, auto &resp) {
    std::string content;
    content = "<h1>Internal Server Error  </h1>";
    content += "<p style=\"color:red\">" +
               std::string(resp.msg_buf()->peek(), resp.msg_buf()->readable()) +
               "</p>";
    resp.status_code(500).body(content).send();
  });
}

void HttpServer::start() {
  InetAddress address(GET_CONFIG(std::string, "server", "listen_ip"),
                      GET_CONFIG(int, "server", "listen_port"));
  server_->start(address);
}

void HttpServer::start(const InetAddress &address) { server_->start(address); }

void HttpServer::set_https_certificate(const std::string &cert_file,
                                       const std::string &private_key_file,
                                       const std::string &password) {
  server_->set_https_certificate(cert_file.c_str(), private_key_file.c_str(),
                                 password.size() > 0 ? password.c_str()
                                                     : nullptr);
}

void HttpServer::quit() { server_->quit(); }

void HttpServer::route(const string &url, const Handler &cb) {
  url_handlers_[url] = cb;
}

void HttpServer::route_regex(const std::string &url_rule, const RMHandler &cb) {
  std::string s = url_rule;
  if (s.empty())
    return;
  if (s.back() != '$')
    s += "$";
  url_rgx_handlers_[s] = std::move(cb);

  if (std::find(url_rules_.begin(), url_rules_.end(), s) == url_rules_.end()) {
    url_rules_.emplace_back(s);
  }
}

void HttpServer::error_code(int code, const Handler &cb) {
  code_handlers_[code] = cb;
}

void HttpServer::redirect(const string &src, const string &dest) {
  if (!src.empty() && !dest.empty())
    redirect_tb_[src] = dest;
}

bool HttpServer::set_mounting_html_dir(const string &url, const string &dir) {
  if (url.empty() || dir.empty())
    return false;

  if (!FileLoader::exist_dir(dir))
    return false;

  std::string url_ = url, dir_ = dir;

  if (dir_.back() != '/')
    dir_ += "/";
  if (url_.back() != '/')
    url_ += "/";
  mount_dir_tb_.emplace_back(url_, dir_);
  std::sort(mount_dir_tb_.begin(), mount_dir_tb_.end(),
            [this](const auto &l, const auto &r) {
              return l.first.size() > r.first.size();
            });
  return true;
}

void HttpServer::set_idle_timeout(int millsecond) {
  if (millsecond <= 0)
    millsecond = 2000;
  server_->set_idle_timeout(millsecond);
}

void HttpServer::set_ext_mimetype_mapping(const std::string &ext,
                                          const std::string &mimetype) {
  mimetype_tb_[hash_ext(ext)] = mimetype;
}
void HttpServer::set_url_auth(const std::string &origin_url,
                              HttpAuthType auth_type) {
  auth_tb_[origin_url] = auth_type;
}

void HttpServer::enable_cgi(bool on) {
  if (on) {
    cgi_bin_ = GET_CONFIG(std::string, "cgi", "cgi_bin");
    enable_cgi_ = on;
  }
}

bool HttpServer::handle_msg(TcpConnectionPtr conn) {
  HttpRequest *req = nullptr;
  if (conn->context() == nullptr) {
    req = new HttpRequest(conn->recver());
    conn->context(static_cast<void *>(req));
  } else {
    req = static_cast<HttpRequest *>(conn->context());
  }

  auto code = req->parse_request();

  if (conn->disconnected() || conn->context() == nullptr) {
    if (req)
      delete req;
    return true;
  }

  HttpResponse resp(conn);
  if (code == HttpRequest::BAD_REQUEST) {
    if (handle_bad_request(req, resp)) {
      delete req;
      // Connection: close
      conn->keep_alive(false);
      conn->context(nullptr);
      return true;
    }
  } else if (code != HttpRequest::REQUEST_CONTENT_DONE)
    return false;

  req->initialize_parse();

  if (!handle_url_auth(req, resp)) {
    delete req;
    conn->context(nullptr);
    return true;
  }
  int state = start_url_route;
  do {
    if (state == start_url_route) {
      if (handle_url_route(req, resp))
        break;
      else
        state = start_url_redirect;
    }
    if (state == start_url_redirect) {
      if (handle_redirect(req, resp))
        break;
      else
        state = start_url_cgi;
    }
    if (state == start_url_cgi) {
      if (enable_cgi_ && handle_cgi_bin(req, resp))
        break;
      else
        state = start_url_locate_resource;
    }
    if (state == start_url_locate_resource) {
      if (handle_static_dynamic_resource(req, resp))
        break;
      else
        state = start_url_route_regex;
    }
    if (state == start_url_route_regex) {
      if (handle_regex_match_url(req, resp))
        break;
      else
        state = start_url_error;
    }
    if (state == start_url_error) {
      code_handlers_[404](*req, resp);
    }
  } while (0);

  delete req;
  conn->context(nullptr);
  return true;
}

bool HttpServer::handle_url_route(HttpRequest *req, HttpResponse &resp) {
  auto it = url_handlers_.find(req->url());
  if (it == url_handlers_.end()) {
    if (it = url_handlers_.find(req->url() + "/"); it == url_handlers_.end()) {
      return false;
    }
  }
  std::string_view vurl = req->url();
  switch (req->method()) {
  case HttpMethod::GET:
  case HttpMethod::POST:
    it->second(*req, resp);
    break;
  case HttpMethod::HEAD:
    resp.send();
    break;
  default:
    break;
  }
  return true;
}

bool HttpServer::handle_redirect(HttpRequest *req, HttpResponse &resp) {
  auto x = redirect_tb_.find(req->url());
  if (x == redirect_tb_.end())
    if (x = redirect_tb_.find(req->url() + "/"); x == redirect_tb_.end())
      return false;

  if (x->second.back() == '/')
    x->second.pop_back();

  resp.status_code(301).header("Location", x->second + req->query_s()).send();
  return true;
}

bool HttpServer::handle_bad_request(HttpRequest *req, HttpResponse &resp) {
  resp.version(HttpVersion::HTTP_1_1).header("Connection", "close");
  resp.status_code(400).body("400 Bad Request!").send();
  return true;
}

bool HttpServer::handle_cgi_bin(HttpRequest *req, HttpResponse &resp) {
  std::string_view vurl = req->url();
  vurl.remove_prefix(1);
  if (!vurl.starts_with(cgi_bin_))
    return false;

  struct stat st;
  int ret = ::stat(vurl.data(), &st);
  if (ret == -1 || !S_ISREG(st.st_mode))
    return false;
  if ((st.st_mode & S_IXUSR) == 0) {
    resp.status_code(500)
        .body("CGI script don't have executable permissions")
        .send();
    return true;
  }

  CGI::execute_cgi_script(vurl, resp);
  resp.send();
  return true;
}

bool HttpServer::handle_url_auth(HttpRequest *req, HttpResponse &resp) {
  if (auth_tb_.empty())
    return true;

  auto item = auth_tb_.find(req->url());
  if (item == auth_tb_.end())
    return true;

  if (item->second == HttpAuthType::Basic) {
    bool verify = false;
    if (GET_CONFIG(bool, "server", "enable_mysql")) {
      auto auth = req->auth<HttpBasicLocalAuth>();
      if (auth && auth->verify_user())
        return true;
    } else {
      auto auth = req->auth<HttpBasicSqlAuth>();
      if (auth && auth->verify_user())
        return true;
    }
  } else if (item->second == HttpAuthType::Digest) {
    if (GET_CONFIG(bool, "server", "enable_mysql")) {
      auto auth = req->auth<HttpDigestSqlAuth>();
      if (auth && auth->verify_user())
        return true;
    } else {
      auto auth = req->auth<HttpDigestLocalAuth>();
      if (auth && auth->verify_user())
        return true;
    }
  }

  resp.auth(item->second).send();
  return false;
}

bool HttpServer::handle_regex_match_url(HttpRequest *req, HttpResponse &resp) {
  if (url_rules_.empty())
    return false;

  std::smatch m;
  std::regex rgx;

  std::string match_url;
  bool match_success = false;
  for (auto &p : url_rules_) {
    rgx = std::regex(p, std::regex::extended | std::regex::ECMAScript);
    if (std::regex_search(req->url(), m, rgx)) {
      match_success = true;
      match_url = p;
      break;
    }
  }

  if (!match_success)
    return false;

  std::vector<std::string> match;
  for (size_t i = 1; i < m.size(); ++i) {
    match.emplace_back(m.str(i));
  }
  url_rgx_handlers_[match_url](*req, resp, match);

  return match_success;
}

bool HttpServer::handle_static_dynamic_resource(HttpRequest *req,
                                                HttpResponse &resp) {
  bool locate_status = false, match_status = false;
  std::string_view prefix, suffix, mount_dir, vurl = req->url();
  std::string local_file_url;
  if (!mount_dir_tb_.empty()) {
    for (auto &dir_url : mount_dir_tb_) {
      if (vurl.starts_with(dir_url.first)) {
        mount_dir = dir_url.second;
        vurl.remove_prefix(dir_url.first.size());
        suffix = vurl;
        match_status = true;
        break;
      }
    }
    if (!match_status)
      return false;
  }
  local_file_url = std::string(mount_dir.data(), mount_dir.size());
  bool enable_php = GET_CONFIG(bool, "server", "enable_php");

  // default index page
  if (suffix.empty()) {
    get_index_page(enable_php, local_file_url);
  } else {
    local_file_url += std::string(suffix.data(), suffix.size());
    // default index page
    if (suffix.back() == '/') {
      get_index_page(enable_php, local_file_url);
    }
  }
  if (FileLoader::exist(local_file_url)) {
    if (local_file_url.ends_with(".php")) {
      if (GET_CONFIG(bool, "server", "enable_php")) {
        return handle_dynamic_resource(local_file_url, req, resp);
      }
    }
    std::string type;
    mapping_mimetype(suffix, type);
    if (type.starts_with("text") || type.ends_with("xml") ||
        type.ends_with("javascript") || type.ends_with("json")) {
      type += " ;charset=utf-8";
    }
    resp.header("Content-Type", type);
    resp.body_file(local_file_url).send();
    locate_status = true;
  }
  return locate_status;
}

bool HttpServer::handle_dynamic_resource(const std::string &relpath,
                                         HttpRequest *req, HttpResponse &resp) {
  PhpFastCgi fcgi;

  if (EXIST_CONFIG("php-fpm", "server_ip")) {
    if (!fcgi.connectPhpFpm(GET_CONFIG(std::string, "php-fpm", "server_ip"),
                            GET_CONFIG(int, "php-fpm", "server_port"))) {
      // error
      return false;
    }
  } else if (EXIST_CONFIG("php-fpm", "sock_path")) {
    if (!fcgi.connectPhpFpm(GET_CONFIG(std::string, "php-fpm", "sock_path"))) {
      // error
      return false;
    }
  } else {
    // error
  }

  fcgi.sendStartRequestRecord();
  fcgi.sendParams(FCGI_Params::SCRIPT_FILENAME,
                  std::filesystem::canonical(relpath));
  if (auto x = req->header().get("Authorization"); x.has_value()) {
    // Basic Authorization
    if (req->auth_type() == HttpAuthType::Basic) {
      std::string username, password;
      if (GET_CONFIG(bool, "server", "enable_mysql")) {
        auto auth = req->auth<HttpBasicSqlAuth>();
        // username & password
        username = auth->username();
        password = auth->password();
      } else {
        auto auth = req->auth<HttpBasicLocalAuth>();
        username = auth->username();
        password = auth->password();
      }
      fcgi.sendParams(FCGI_Params::PHP_AUTH_USER, username);
      fcgi.sendParams(FCGI_Params::PHP_AUTH_PW, password);
      // Digest Authorization
    } else if (req->auth_type() == HttpAuthType::Digest) {
      std::string digest_info;
      if (GET_CONFIG(bool, "server", "enable_mysql")) {
        auto auth = req->auth<HttpDigestSqlAuth>();
        digest_info = auth->digest_info();
      } else {
        auto auth = req->auth<HttpDigestLocalAuth>();
        digest_info = auth->digest_info();
      }
      fcgi.sendParams(FCGI_Params::PHP_AUTH_DIGEST, digest_info);
    }
  }

  if (req->has_query()) {
    fcgi.sendParams(FCGI_Params::QUERY_STRING, req->query_s());
  }
  if (req->method() == HttpMethod::HEAD) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "HEAD");
    fcgi.sendEndRequestRecord();
  } else if (req->method() == HttpMethod::GET) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "GET");
    fcgi.sendEndRequestRecord();
  } else if (req->method() == HttpMethod::POST) {
    fcgi.sendParams(FCGI_Params::REQUEST_METHOD, "POST");
    if (auto type = req->header().get("Content-Type"); type.has_value()) {
      fcgi.sendParams(FCGI_Params::CONTENT_TYPE, type.value());
      fcgi.sendParams(FCGI_Params::CONTENT_LENGTH,
                      std::to_string(req->post_data().size()));
    }
    fcgi.sendEndRequestRecord();
    fcgi.sendPost(req->post_data());
  }

  Buffer buffer;
  auto ret = fcgi.readPhpFpm(&buffer);
  std::string_view sv(buffer.peek(), buffer.readable());
  HttpHeader header;
  int index = header.parse(sv);

  if (auto x = header.get("Status"); x.has_value()) {
    std::string_view status = x.value();
    std::string_view codesv = status.substr(0, status.find_first_of(" "));
    header.remove("Status");

    int code = std::atoi(std::string(codesv.data(), codesv.size()).data());
    resp.status_code(code);
    if (code == 500) {
      // Server Internal Error
      if (auto x = code_handlers_.find(code); x != code_handlers_.end()) {
        resp.header(header);
        resp.msg_buf()->append(ret.second.data(), ret.second.size());
        x->second(*req, resp);
        return true;
      }
    }
  }
  resp.header(header);
  sv.remove_prefix(index);
  resp.append_body(sv);
  resp.send();
  return true;
}

void HttpServer::mapping_mimetype(const std::string_view &url,
                                  std::string &type) {
  size_t pos = url.find_first_of(".");
  std::string_view ext = url.substr(pos + 1);
  uint64_t hashv = hash_ext(ext);
  if (auto t = Content_Type.find(hashv); t != Content_Type.end())
    type = t->second;
  else if (auto t = mimetype_tb_.find(hashv); t != mimetype_tb_.end()) {
    type = t->second;
  } else
    type = "*/*"; // or text/plain
}

void HttpServer::get_index_page(bool php, std::string &rd) {
  for (auto &index_page : default_page_) {
    if (!php && index_page.ends_with("php"))
      continue;
    if (std::filesystem::exists(rd + index_page)) {
      rd += index_page;
    }
  }
}