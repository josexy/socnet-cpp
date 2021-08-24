#include "../include/HttpRequest.h"
using namespace soc::http;

HttpRequest::HttpRequest(net::TcpConnection *conn, HttpSessionServer *owner)
    : recver_(conn->getRecver()), owner_(owner),
      remote_addr_(conn->getPeerAddr()), keepalive_(false), compressed_(false),
      has_multipart_(false), has_cookies_(false), auth_(nullptr),
      session_(nullptr) {
  reset();
}

HttpRequest::~HttpRequest() {
  if (auth_)
    delete auth_;
  auth_ = nullptr;
  session_ = nullptr;
}

void HttpRequest::reset() { ret_code_ = NO_REQUEST; }

void HttpRequest::setHeader(const std::string &key, const std::string &value) {
  header_.add(key, value);
}

std::optional<std::string>
HttpRequest::getHeaderValue(const std::string &key) const {
  return header_.get(key);
}

std::string HttpRequest::getFullUrl() const noexcept {
  static const bool on_https = GET_CONFIG(bool, "server", "enable_https");
  static const bool has_host = EXIST_CONFIG("server", "server_hostname");
  static const std::string schema = on_https ? "https" : "http";
  static const int port = GET_CONFIG(int, "server", "listen_port");
  std::string host;
  if (has_host) {
    host = GET_CONFIG(std::string, "server", "server_hostname");
  } else {
    host = GET_CONFIG(std::string, "server", "listen_ip");
  }
  return schema + ":" + "//" + host + ":" + std::to_string(port) +
         EncodeUtil::urlDecode(req_url_.data(), req_url_.size());
}

HttpSession *HttpRequest::getSession() const {
  session_ = owner_->associateSession(const_cast<HttpRequest *>(this));
  return session_;
}

HttpRequest::RetCode HttpRequest::parseRequest() {
  while (true) {
    switch (ret_code_) {
    case NO_REQUEST: {
      parseRequestLine();
      break;
    }
    case REQUEST_LINE_DONE:
    case AGAIN_HEADER: {
      auto code = parseRequestHeader();
      if (code == AGAIN_HEADER)
        return code;
      break;
    }
    case REQUEST_HEADER_DONE: {
      parseUrlQuery();

      if (header_.empty() && version_ != HttpVersion::HTTP_1_0) {
        keepalive_ = true;
        return REQUEST_CONTENT_DONE;
      }

      if (version_ != HttpVersion::HTTP_1_0)
        keepalive_ = true;

      if (auto conn = header_.get("Connection"); conn.has_value()) {
        // Before HTTP/1.0, the Connection field in Request or Response need
        // present. After HTTP/1.1, the Conection do not present in request and
        // response header Connection field
        std::string_view v = EncodeUtil::toLowers(conn.value());
        if (version_ == HttpVersion::HTTP_1_0 && v.starts_with("keep-alive"))
          keepalive_ = true;
        else if (v.starts_with("close"))
          keepalive_ = false;
      }

      if (auto enc = header_.get("Accept-Encoding"); enc.has_value()) {
        compressed_ = std::string_view(enc.value()).find("gzip") !=
                      std::string_view::npos;
      }

      parseCookie();
      parseMultiPartData();
      parseAuthorization();

      auto code = parseRequestContent();
      if (code == AGAIN_CONTENT)
        return code;
      break;
    }
    case AGAIN_CONTENT: {
      auto code = parseRequestContent();
      if (code == AGAIN_CONTENT)
        return code;
      break;
    }
    case BAD_REQUEST:
    case REQUEST_CONTENT_DONE:
      return ret_code_;
    }
  }
}

HttpRequest::RetCode HttpRequest::parseRequestLine() {
  size_t i = 0;
  std::string_view line(recver_->peek(), recver_->readable());

  if (line.starts_with("GET"))
    method_ = HttpMethod::GET, i += 4;
  else if (line.starts_with("POST"))
    method_ = HttpMethod::POST, i += 5;
  else if (line.starts_with("HEAD"))
    method_ = HttpMethod::HEAD, i += 5;
  else
    return ret_code_ = BAD_REQUEST;
  line.remove_prefix(i);

  // Parse request line url
  size_t pos = line.find_first_of(' ');
  if (pos == line.npos)
    return ret_code_ = BAD_REQUEST;
  req_url_ = std::string(line.data(), pos);

  line.remove_prefix(pos + 1);
  i += pos + 1;

  // Parse request line version
  if (line.starts_with("HTTP/1.0"))
    version_ = HttpVersion::HTTP_1_0;
  else if (line.starts_with("HTTP/1.1"))
    version_ = HttpVersion::HTTP_1_1;
  else if (line.starts_with("HTTP/2.0"))
    version_ = HttpVersion::HTTP_2_0;
  else
    return ret_code_ = BAD_REQUEST;

  line.remove_prefix(10);
  i += 10;
  recver_->retired(i);
  return ret_code_ = REQUEST_LINE_DONE;
}

HttpRequest::RetCode HttpRequest::parseRequestHeader() {
  std::string_view header(recver_->peek(), recver_->readable());
  std::string_view line_header;
  while (true) {
    if (header.size() >= 2 && header[0] == CR && header[1] == LF) {
      recver_->retired(2);
      // \r\n
      return ret_code_ = REQUEST_HEADER_DONE;
    }
    size_t pos = header.find("\r\n");
    if (pos == header.npos)
      break;

    line_header = header.substr(0, pos);

    size_t pos2 = line_header.find_first_of(":");
    if (pos2 == line_header.npos)
      return ret_code_ = BAD_REQUEST;

    std::string_view key = line_header.substr(0, pos2);
    std::string_view value = line_header.substr(pos2 + 2, pos - pos2 - 2);
    header_.add(std::string(key.data(), key.size()),
                std::string(value.data(), value.size()));

    header.remove_prefix(pos + 2);
    recver_->retired(pos + 2);
  }
  return ret_code_ = AGAIN_HEADER;
}

HttpRequest::RetCode HttpRequest::parseRequestContent() {
  std::string_view content(recver_->peek(), recver_->readable());
  post_data_.append(content.data(), content.size());
  // Content-Type
  // 1. application/x-www-form-urlencoded
  // 2. multipart/form-data
  if (has_multipart_) {
    std::string eob = "--" + multipart_.getBoundary() + "--\r\n";
    if (!content.ends_with(eob))
      return ret_code_ = AGAIN_CONTENT;
    multipart_.parse(content);
  } else {
    parseKeyValue(content, "=", "&", form_);
  }
  recver_->retiredAll();
  return ret_code_ = REQUEST_CONTENT_DONE;
}

void HttpRequest::parseUrlQuery() {
  std::string_view uls(req_url_.data(), req_url_.size());
  size_t pos = uls.find_first_of("?");
  if (pos != uls.npos) {
    query_s_ = uls.substr(pos + 1);
    uls.remove_prefix(pos + 1);
    parseKeyValue(uls, "=", "&", query_);
  }
  url_ = EncodeUtil::urlDecode(req_url_.substr(0, pos));
}

void HttpRequest::parseCookie() {
  if (auto it = header_.get("Cookie"); it.has_value()) {
    has_cookies_ = true;
    parseKeyValue(it.value(), "=", ";", cookies_);
  }
}

void HttpRequest::parseMultiPartData() {
  if (auto it = header_.get("Content-Type"); it.has_value()) {
    std::string_view value = it.value();
    if (value.starts_with("multipart/form-data")) {
      has_multipart_ = true;
      value.remove_prefix(21);
      if (value.starts_with("boundary")) {
        value.remove_prefix(9);
        multipart_.setBoundary(std::string(value.data(), value.size()));
      }
    }
  }
}

void HttpRequest::parseAuthorization() {
  if (auto x = header_.get("Authorization"); x.has_value()) {
    std::string_view auth = x.value();
    if (auth.starts_with("Basic")) {
      auth.remove_prefix(6);

      std::string user_pass = EncodeUtil::base64Decode(auth);
      std::string_view user_pass_sv(user_pass.data(), user_pass.size());

      size_t pos = user_pass.find_first_of(":");
      std::string_view user = user_pass_sv.substr(0, pos);
      std::string_view pass = user_pass_sv.substr(pos + 1);

      auth_ = new HttpBasicAuth(user, pass);

    } else if (auth.starts_with("Digest")) {
      auth.remove_prefix(7);

      int state = 0;
      size_t i = 0, k = 0, v = 0, len = auth.size();
      char buffer[128]{0};

      while (i < len) {
        switch (state) {
        case 0: {
          while (i < len) {
            if (auth[i] == '=') {
              ++i;
              v = k;
              state = 1;
              break;
            }
            buffer[k++] = auth[i++];
          }
          break;
        }
        case 1: {
          if (auth[i] == '\"')
            ++i;
          while (i < len) {
            if (auth[i] == ',') {
              break;
            } else if (auth[i] == '\"') {
              ++i;
              break;
            }
            buffer[v++] = auth[i++];
          }
          da_.add(std::string(buffer, k), std::string(buffer + k, v - k));
          k = v = 0;
          state = 2;
          break;
        }
        case 2: {
          if (i >= len)
            break;
          if (i + 1 < len && auth[i] == ',' && auth[i + 1] == ' ') {
            i += 2;
            state = 0;
          }
          break;
        }
        default:
          ++i;
          break;
        }
      }

      std::string method;
      if (method_ == HttpMethod::GET)
        method = "GET";
      else if (method_ == HttpMethod::POST)
        method = "POST";
      else if (method_ == HttpMethod::HEAD)
        method = "HEAD";
      da_.add("method", method);
      auth_ = new HttpDigestAuth(x.value(), da_);
    }
  } else {
    // other authorization...
  }
}

void HttpRequest::parseKeyValue(std::string_view s, const std::string_view &s1,
                                const std::string_view &s2,
                                HttpMap<std::string, std::string> &c) {
  while (!s.empty()) {
    size_t pos = s.find(s2);
    std::string_view kv = s.substr(0, pos);
    size_t pos2 = kv.find(s1);
    c.add(EncodeUtil::urlDecode(std::string(kv.substr(0, pos2))),
          EncodeUtil::urlDecode(std::string(
              kv.substr(pos2 + s1.size(), pos - pos2 - s1.size()))));
    if (pos == s.npos)
      pos = s.size();
    else {
      while (kv[pos + 1] == ' ')
        pos++;
      pos += s2.size();
    }
    s.remove_prefix(pos);
  }
}
