
#include "../include/HttpRequest.h"

using namespace soc::http;

HttpRequest::HttpRequest(net::Buffer *recver)
    : recver_(recver), close_(true), compressed_(false), has_multipart_(false),
      has_cookies_(false) {
  initialize_parse();
}

void HttpRequest::initialize_parse() { ret_code_ = NO_REQUEST; }

void HttpRequest::add(const std::string &key, const std::string &value) {
  header_.add(key, value);
}

std::optional<std::string> HttpRequest::get(const std::string &key) const {
  return header_.get(key);
}

std::string HttpRequest::full_url() const noexcept {
  return Env::instance().get("SCHEMA").value() + "//" +
         Env::instance().get("SERVER_ADDR").value() +
         EncodeUtil::url_decode(req_url_.c_str(), req_url_.size());
}

HttpRequest::RetCode HttpRequest::parse_request() {
  while (true) {
    switch (ret_code_) {
    case NO_REQUEST: {
      parse_request_line();
      break;
    }
    case REQUEST_LINE_DONE:
    case AGAIN_HEADER: {
      auto code = parse_request_header();
      if (code == AGAIN_HEADER)
        return code;
      break;
    }
    case REQUEST_HEADER_DONE: {
      parse_url_query();

      if (header_.count() == 0 && version_ != HttpVersion::HTTP_1_0) {
        close_ = false;
        return REQUEST_CONTENT_DONE;
      }

      if (version_ != HttpVersion::HTTP_1_0)
        close_ = false;

      if (auto conn = header_.get("Connection"); conn.has_value()) {
        // Before HTTP/1.0, the Connection field in Request or Response need
        // present. After HTTP/1.1, the Conection do not present in request and
        // response header Connection field
        std::string_view v = conn.value();
        if (version_ == HttpVersion::HTTP_1_0 && v.starts_with("keep-alive"))
          close_ = false;
        else if (v.starts_with("close"))
          close_ = true;
      }

      if (auto enc = header_.get("Accept-Encoding"); enc.has_value()) {
        compressed_ = std::string_view(enc.value()).find("gzip") !=
                      std::string_view::npos;
      }
      parse_authorization();

      auto code = parse_request_content();
      if (code == AGAIN_CONTENT)
        return code;
      break;
    }
    case AGAIN_CONTENT: {
      auto code = parse_request_content();
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

HttpRequest::RetCode HttpRequest::parse_request_line() {
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

HttpRequest::RetCode HttpRequest::parse_request_header() {
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

    if (!has_multipart_ && key == "Content-Type" &&
        value.starts_with("multipart/form-data")) {
      has_multipart_ = true;
      value.remove_prefix(21);
      if (value.starts_with("boundary")) {
        value.remove_prefix(9);
        multipart_.set_boundary(std::string(value.data(), value.size()));
      }
    } else if (!has_cookies_ && key == "Cookie") {
      has_cookies_ = true;
      parse_key_value(value, "=", "; ", cookies_);
    }
    header.remove_prefix(pos + 2);
    recver_->retired(pos + 2);
  }
  return ret_code_ = AGAIN_HEADER;
}

HttpRequest::RetCode HttpRequest::parse_request_content() {
  std::string_view content(recver_->peek(), recver_->readable());
  // Content-Type
  // 1. application/x-www-form-urlencoded
  // 2. multipart/form-data
  if (has_multipart_) {
    std::string eob = "--" + multipart_.boundary() + "--\r\n";
    if (!content.ends_with(eob))
      return ret_code_ = AGAIN_CONTENT;
    multipart_.parse(content);
  } else {
    parse_key_value(content, "=", "&", form_);
  }
  recver_->retired_all();
  return ret_code_ = REQUEST_CONTENT_DONE;
}

void HttpRequest::parse_url_query() {
  std::string_view uls(req_url_.data(), req_url_.size());
  size_t pos = uls.find_first_of("?");
  if (pos != uls.npos) {
    query_s_ = uls.substr(pos);
    uls.remove_prefix(pos + 1);
    parse_key_value(uls, "=", "&", query_);
  }
  url_ = EncodeUtil::url_decode(req_url_.substr(0, pos));
}

void HttpRequest::parse_authorization() {
  if (auto x = header_.get("Authorization"); x.has_value()) {
    std::string_view auth = x.value();
    if (auth.starts_with("Basic")) {
      auth.remove_prefix(6);

      auth_ = HttpBasicAuthType();
      auto &basic_auth = std::get<HttpBasicAuthType>(auth_);

      std::string user_pass = EncodeUtil::base64_decode(auth);
      std::string_view user_pass_sv(user_pass.data(), user_pass.size());

      size_t pos = user_pass.find_first_of(":");
      std::string_view user = user_pass_sv.substr(0, pos);
      std::string_view pass = user_pass_sv.substr(pos + 1);

      basic_auth.verify_user_private(user, pass);
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
          da_.emplace(std::string(buffer, k), std::string(buffer + k, v - k));
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

      auth_ = HttpDigestAuthType();
      auto &digest_auth = std::get<HttpDigestAuthType>(auth_);
      std::string_view method;
      if (method_ == HttpMethod::GET)
        method = "GET";
      else if (method_ == HttpMethod::POST)
        method = "POST";
      else if (method_ == HttpMethod::HEAD)
        method = "HEAD";

      digest_auth.verify_user_private(
          da_.count("username") ? da_["username"] : "", method,
          da_.count("uri") ? da_["uri"] : "",
          da_.count("nonce") ? da_["nonce"] : "",
          da_.count("nc") ? da_["nc"] : "",
          da_.count("cnonce") ? da_["cnonce"] : "",
          da_.count("qop") ? da_["qop"] : "",
          da_.count("response") ? da_["response"] : "");
    } else {
      // other authorization...
    }
  }
}

void HttpRequest::parse_key_value(
    std::string_view s, const std::string_view &s1, const std::string_view &s2,
    std::unordered_map<std::string, std::string> &c) {
  while (!s.empty()) {
    size_t pos = s.find(s2);
    std::string_view kv = s.substr(0, pos);
    size_t pos2 = kv.find(s1);
    c.emplace(EncodeUtil::url_decode(std::string(kv.substr(0, pos2))),
              EncodeUtil::url_decode(std::string(
                  kv.substr(pos2 + s1.size(), pos - pos2 - s1.size()))));
    if (pos == s.npos)
      pos = s.size();
    else
      pos += s2.size();
    s.remove_prefix(pos);
  }
}
