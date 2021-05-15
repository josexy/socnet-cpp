
#include "../include/HttpResponse.h"

#include "../../utility/include/timestamp.h"

using namespace soc::http;

HttpResponseBuilder::HttpResponseBuilder(HttpRequest *request,
                                         net::Buffer *sender)
    : uri_(request->url()), version_(request->version()), resp_file_(false),
      close_(request->close_conn()), compressed_(request->compressed()),
      status_code_(200), sender_(sender) {
  header_.add("Server", "socnet");
  header_.add("Content-Type", "text/plain; charset=utf-8");
  header_.add("Date", soc::timestamp::server_date());

  tmp_buffer_.retired_all();
}

HttpResponseBuilder &HttpResponseBuilder::auth(HttpAuthType auth_type) {
  status_code_ = 401;
  char buffer[1024]{0};
  if (auth_type == HttpAuthType::Basic) {
    ::sprintf(buffer, "Basic realm=\"%s\"", kRealm);
  } else if (auth_type == HttpAuthType::Digest) {
    ::sprintf(buffer,
              "Digest realm=\"%s\", qop=\"auth,auth-int\", nonce=\"%s\", "
              "opaque=\"%s\"",
              kRealm,
              EncodeUtil::base64_encode(EncodeUtil::generate_randrom_string(16))
                  .data(),
              EncodeUtil::base64_encode(EncodeUtil::generate_randrom_string(16))
                  .data());
  }
  header_.add("WWW-Authenticate", buffer);
  return *this;
}

void HttpResponseBuilder::prepare_header() {
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

  std::string code = std::to_string(status_code_) + " ";
  sender_->append(code.c_str(), code.size());

  std::string_view message = Status_Code.at(status_code_);
  sender_->append(message.data(), message.size());
  sender_->append("\r\n", 2);

  header_.store(sender_);
  sender_->append("\r\n", 2);
}

void HttpResponseBuilder::build() {
  // Only compresses responses for the following mime types:
  // text/*
  // */*+json
  // */*+text
  // */*+xml

  auto x = header_.get("Content-Type");
  if (x.has_value()) {
    std::string_view type = x.value();
    if (type.starts_with("text") || type.ends_with("xml") ||
        type.ends_with("javascript") || type.ends_with("json")) {
    } else {
      compressed_ = false;
    }
  } else {
    compressed_ = false;
  }

  auto func_connection = [this](size_t content_length) {
    if (version_ == HttpVersion::HTTP_1_0) {
      if (close_) {
        header_.add("Connection", "close");
      } else {
        header_.add("Connection", "keep-alive");
      }
    } else if (close_) {
      header_.add("Connection", "close");
    }
    if (!close_) {
      // not close
      header_.add("Content-Length", std::to_string(content_length));
    }
  };
  if (resp_file_) {
    FileLoader(file_name_).read_all(&tmp_buffer_);
  }
  if (compressed_) {
    header_.add("Content-Encoding", "gzip");
    EncodeUtil::gzipcompress(
        &tmp_buffer_, sender_, func_connection,
        std::bind(&HttpResponseBuilder::prepare_header, this));

  } else {
    func_connection(tmp_buffer_.readable());
    prepare_header();
    sender_->append(tmp_buffer_.peek(), tmp_buffer_.readable());
  }
}

HttpResponse::HttpResponse(net::TcpConnectionPtr conn)
    : conn_(conn),
      builder_(new HttpResponseBuilder(
          static_cast<HttpRequest *>(conn->context()), conn->sender())) {}

void HttpResponse::send() {
  if (conn_ == nullptr)
    return;
  conn_->keep_alive(!builder_->close());
  builder_->build();
}
