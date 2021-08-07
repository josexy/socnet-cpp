#include "../include/HttpResponse.h"
#include "../../net/include/TimeStamp.h"

using namespace soc::http;

HttpResponseBuilder::HttpResponseBuilder(HttpRequest *request,
                                         net::Buffer *sender)
    : uri_(request->url()), version_(request->version()), resp_file_(false),
      keepalive_(request->isKeepAlive()), compressed_(request->isCompressed()),
      code_(200), sender_(sender) {
  header_.add("Server", "socnet");
  header_.add("Content-Type", "text/plain; charset=utf-8");
  header_.add("Date", soc::net::TimeStamp::serverDate());

  tmp_buffer_.retiredAll();
}

HttpResponseBuilder &HttpResponseBuilder::auth(HttpAuthType type) {
  code_ = 401;
  char buffer[1024]{0};
  if (type == HttpAuthType::Basic) {
    ::sprintf(buffer, "Basic realm=\"%s\"", kRealm);
  } else if (type == HttpAuthType::Digest) {
    ::sprintf(buffer,
              "Digest realm=\"%s\", qop=\"auth,auth-int\", nonce=\"%s\", "
              "opaque=\"%s\"",
              kRealm,
              EncodeUtil::base64Encode(EncodeUtil::genRandromStr(16)).data(),
              EncodeUtil::base64Encode(EncodeUtil::genRandromStr(16)).data());
  }
  header_.add("WWW-Authenticate", buffer);

  return *this;
}

void HttpResponseBuilder::prepareHeader() {
  switch (version_) {
  case HttpVersion::HTTP_1_0:
    sender_->append("HTTP/1.0 ", 9);
    break;
  case HttpVersion::HTTP_1_1:
    sender_->append("HTTP/1.1 ", 9);
    break;
  case HttpVersion::HTTP_2_0:
    sender_->append("HTTP/2.0 ", 9);
    break;
  default:
    break;
  }

  std::string code = std::to_string(code_) + " ";
  sender_->append(code.c_str(), code.size());

  std::string_view message = Status_Code.at(code_);
  sender_->append(message.data(), message.size());
  sender_->append("\r\n", 2);

  header_.store(sender_);
  sender_->append("\r\n", 2);
}

void HttpResponseBuilder::makeHeaderPart(size_t content_length) {
  if (version_ == HttpVersion::HTTP_1_0) {
    if (keepalive_) {
      header_.add("Connection", "keep-alive");
    } else {
      header_.add("Connection", "close");
    }
  } else if (!keepalive_) {
    header_.add("Connection", "close");
  }
  if (keepalive_) {
    header_.add("Content-Length", std::to_string(content_length));
  }
}

void HttpResponseBuilder::build() {
  // Only compresses responses for the following mime types:
  // text/*
  // */*+json
  // */*+text
  // */*+xml

  if (auto x = header_.get("Content-Type"); x.has_value()) {
    std::string_view type = x.value();
    if (type.starts_with("text") || type.ends_with("xml") ||
        type.ends_with("javascript") || type.ends_with("json")) {
    } else {
      compressed_ = false;
    }
  } else {
    compressed_ = false;
  }

  std::string_view sv;
  if (resp_file_)
    sv = FileUtil(file_name_).readAll(&tmp_buffer_);
  else
    sv = std::string_view(tmp_buffer_.peek(), tmp_buffer_.readable());

  std::vector<uint8_t> out;
  if (compressed_) {
    header_.add("Content-Encoding", "gzip");
    EncodeUtil::gzipCompress(sv, out);
    makeHeaderPart(out.size());
  } else
    makeHeaderPart(sv.size());

  prepareHeader();
  if (compressed_) {
    sender_->append<uint8_t>(out.begin(), out.end());
  } else {
    if (tmp_buffer_.mappingAddr()) {
      sender_->appendMapping(sv);
      tmp_buffer_.appendMapping(nullptr, 0);
    } else
      sender_->append(sv);
  }
}

HttpResponse::HttpResponse(net::TcpConnection *conn)
    : conn_(conn),
      builder_(new HttpResponseBuilder(
          static_cast<HttpRequest *>(conn->context()), conn->sender())) {}

void HttpResponse::sendAuth(HttpAuthType type) { builder_->auth(type); }

void HttpResponse::sendRedirect(const std::string &url) {
  this->code(302).header("Location", url);
}

void HttpResponse::send() {
  if (conn_ == nullptr)
    return;
  conn_->setKeepAlive(builder_->keepAlive());
  builder_->build();
}
