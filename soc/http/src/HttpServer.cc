
#include "../include/HttpServer.h"
#include "../../modules/cgi/include/CGI.h"
#include "../../utility/include/FileLister.h"
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
  set_idle_timeout(0);

  server_->set_msg_cb(std::bind(&HttpServer::handle_msg, this, _1));

  code_handlers_.emplace(404, [](const auto & /*req*/, auto &resp) {
    resp.status_code(404).body("404 not found!").send();
  });
}

void HttpServer::start(const InetAddress &address) {
  initialize_env();
  Env::instance().set("SERVER_ADDR", address.to_string());
  server_->start(address);
}

#ifdef SUPPORT_SSL_LIB

void HttpServer::set_https_certificate(const char *cert_file,
                                       const char *private_key_file,
                                       const char *password) {
  server_->set_https_certificate(cert_file, private_key_file, password);
}
#endif

void HttpServer::initialize_env() {
#ifdef SUPPORT_SSL_LIB
  Env::instance().set("SCHEMA", "https");
#else
  Env::instance().set("SCHEMA", "http");
#endif
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

bool HttpServer::set_mounting_file_dir(const string &url) {
  if (url.empty())
    return false;

  if (url.front() == '/' && url.size() > 1)
    if (!FileLoader::exist_dir(std::string(url.c_str() + 1)))
      return false;

  if (url.back() != '/')
    fdir_tb_.emplace_back(url + "/");
  else
    fdir_tb_.emplace_back(url);

  std::sort(
      fdir_tb_.begin(), fdir_tb_.end(),
      [this](const auto &l, const auto &r) { return l.size() > r.size(); });
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
  auto op = Env::instance().get("CGI-BIN");
  assert(op.has_value());
  cgi_bin_ = op.value();
  enable_cgi_ = on;
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
  bool is_reg_file = false;
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
        state = start_url_file_directory;
    }
    if (state == start_url_file_directory) {
      if (handle_file_dir_index_of(is_reg_file, req, resp))
        break;
      else
        state = start_url_locate_resource;
    }
    if (state == start_url_locate_resource) {
      if (handle_static_resource(is_reg_file, req, resp))
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

bool HttpServer::handle_file_dir_index_of(bool &is_reg_file, HttpRequest *req,
                                          HttpResponse &resp) {
  if (fdir_tb_.empty())
    return false;
  std::string_view vurl = req->url();

  if (vurl.find_first_not_of("/") == vurl.npos)
    return false;

  struct stat st;
  int ret = stat(vurl.data() + 1, &st);
  if (ret == -1) {
    if (vurl != "/" || fdir_tb_.back() != vurl)
      return false;
  } else if (S_ISREG(st.st_mode)) {
    is_reg_file = true;
    return false;
  }

  if (vurl.back() != '/') {
    resp.status_code(301)
        .header("Location", std::string(vurl.data(), vurl.size()) + "/")
        .send();
    return true;
  }

  bool found = false;
  for (auto &dir_url : fdir_tb_) {
    if (vurl.starts_with(dir_url)) {
      found = true;
      break;
    }
  }
  if (!found) {
    if (auto x = code_handlers_.find(403); x != code_handlers_.end())
      x->second(*req, resp);
    else
      resp.status_code(403).body("Forbidden to access site!").send();
    return false;
  }

  std::string title = "Index of " + req->url();
  std::string file_img = "/resources/text.gif",
              dir_img = "/resources/folder.gif";
  std::string document = R"(
<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8">)";

  document += "<title>" + title + "</title>" + "</head>" + "<body>" + "<h1>" +
              title + "</h1>";
  document += R"(
<table><tr>
<th><img src=""></th>
<th>Name</th>
<th>Last modified</th>
<th>Size</th>
</tr>
<tr><th colspan=5><hr/></th></tr>)";

  std::vector<FileLister::FileInfo> files;
  if (vurl.size() == 1)
    vurl = ".";
  else
    vurl.remove_prefix(1);
  FileLister::list(vurl, files);
  for (auto &file : files) {
    std::string img = file.is_dir ? dir_img : file_img;
    document += "<tr><td><img src=\"" + img + "\" ></td><td><a href=\"" +
                file.name + "\">" + file.name + "</a></td><td>" +
                timestamp::fmt_time_t(file.lastmodtime) + "</td><td>" +
                std::to_string(file.size) + "</td></tr>\n";
  }

  document += R"(<tr><th colspan="5"><hr/></th></tr></table></body></html>)";
  resp.header("Content-Type", "text/html; charset=utf-8").body(document).send();
  return true;
}

bool HttpServer::handle_url_auth(HttpRequest *req, HttpResponse &resp) {
  if (auth_tb_.empty())
    return true;

  auto item = auth_tb_.find(req->url());
  if (item == auth_tb_.end())
    return true;

  if (item->second == HttpAuthType::Basic) {
    HttpAuth<HttpBasicAuthType> *auth = req->auth<HttpBasicAuthType>();
    if (auth && auth->verify_user()) {
      return true;
    }
  } else if (item->second == HttpAuthType::Digest) {
    HttpAuth<HttpDigestAuthType> *auth = req->auth<HttpDigestAuthType>();
    if (auth && auth->verify_user()) {
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

bool HttpServer::handle_static_resource(bool is_reg_file, HttpRequest *req,
                                        HttpResponse &resp) {
  bool locate_status = false, match_status = false;
  std::string_view prefix, suffix, mount_dir, vurl = req->url();
  std::string local_file_url;

  if (!is_reg_file) {
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
    if (suffix.empty())
      suffix = "index.html";

    local_file_url = std::string(mount_dir.data(), mount_dir.size()) +
                     std::string(suffix.data(), suffix.size());
  } else {
    vurl.remove_prefix(1);
    local_file_url = std::string(vurl.data(), vurl.size());
    size_t pos;
    if ((pos = vurl.find_last_of("/")) == vurl.npos)
      return locate_status;
    suffix = vurl.substr(pos + 1);
  }
  if (FileLoader::exist(local_file_url)) {
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
