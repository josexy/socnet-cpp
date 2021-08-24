#include "../include/HttpResponse.h"
#include "../../net/include/TimeStamp.h"

using namespace soc::http;

HttpResponseBuilder::HttpResponseBuilder(HttpRequest *request,
                                         net::TcpConnection *conn)
    : uri_(request->getUrl()), version_(request->getVersion()),
      method_(request->getMethod()), resp_file_(false),
      keepalive_(request->isKeepAlive()), compressed_(request->isCompressed()),
      code_(HttpStatus::OK), conn_(conn) {
  header_.add("Server", "socnet");
  header_.add("Content-Type", "application/octet-stream");
  header_.add("Date", soc::net::TimeStamp::getServerDate());
  tmp_buffer_.retiredAll();
}

HttpResponseBuilder &HttpResponseBuilder::setAuthType(HttpAuthType type) {
  static const std::string realm = HttpPassStore::instance().getRealm();
  code_ = HttpStatus::UNAUTHORIZED;
  char buffer[1024]{0};
  if (type == HttpAuthType::Basic) {
    ::sprintf(buffer, "Basic realm=\"%s\"", realm.data());
  } else if (type == HttpAuthType::Digest) {
    char nonce[17]{0}, opaque[17]{0};
    EncodeUtil::genRandromStr(nonce);
    EncodeUtil::genRandromStr(opaque);
    ::sprintf(buffer,
              "Digest realm=\"%s\", qop=\"auth,auth-int\", nonce=\"%s\", "
              "opaque=\"%s\"",
              realm.data(), EncodeUtil::base64Encode(nonce).data(),
              EncodeUtil::base64Encode(opaque).data());
  }
  header_.add("WWW-Authenticate", buffer);
  header_.add("Content-Type", "text/plain; charset=utf-8");
  return *this;
}

void HttpResponseBuilder::prepareHeader() {
  switch (version_) {
  case HttpVersion::HTTP_1_0:
    conn_->getSender()->append("HTTP/1.0 ", 9);
    break;
  case HttpVersion::HTTP_1_1:
    conn_->getSender()->append("HTTP/1.1 ", 9);
    break;
  case HttpVersion::HTTP_2_0:
    conn_->getSender()->append("HTTP/2.0 ", 9);
    break;
  default:
    break;
  }

  std::string code = std::to_string(code_) + " ";
  conn_->getSender()->append(code.c_str(), code.size());

  std::string_view message = Status_Code.at(code_);
  conn_->getSender()->append(message.data(), message.size());
  conn_->getSender()->append("\r\n", 2);

  header_.store(conn_->getSender());
  conn_->getSender()->append("\r\n", 2);
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

  std::pair<char *, size_t> sv;
  bool putinto_buf = true;
  if (resp_file_) {
    int infd = FileUtil::openFile(file_name_);
    struct stat st;
    ::fstat(infd, &st);
    long size = st.st_size;
    char mtime[50]{0};
    ::strftime(mtime, 50, "%a, %d %b %Y %H:%M:%S GMT", ::gmtime(&st.st_mtime));
    setHeader("Last-Modified", mtime);

    // sendfile()
    // not support dynamic gzip
    if (conn_->getChannel()->supportSendFile()) {
      conn_->getChannel()->createSendFileObject(infd, size);
      compressed_ = false;
      putinto_buf = false;
      sv = std::make_pair(nullptr, size);
    } else {
      // 4M
      constexpr static size_t M = 4 * 1024 * 1024;
      // read()
      if (size <= M) {
        tmp_buffer_.ensureWritable(size);
        FileUtil::read(infd, tmp_buffer_.beginWrite(), size);
        FileUtil::closeFile(infd);
        tmp_buffer_.hasWritten(size);
        sv = std::make_pair(tmp_buffer_.beginRead(), tmp_buffer_.readable());
      } else {
        // mmap()
        const auto x = conn_->getChannel()->createMMapObject(infd, size);
        FileUtil::closeFile(infd);
        sv = std::make_pair(x->getAddress(), size);
        putinto_buf = false;
      }
    }
  } else {
    sv = std::make_pair(tmp_buffer_.beginRead(), tmp_buffer_.readable());
  }

  std::vector<uint8_t> out;
  if (compressed_ && sv.second) {
    // sendfile() not support gzip compress
    header_.add("Content-Encoding", "gzip");
    EncodeUtil::gzipCompress(std::string_view(sv.first, sv.second), out);
    sv.second = out.size();
  }

  makeHeaderPart(sv.second);
  prepareHeader();
  // HEAD method
  if (method_ == HttpMethod::HEAD)
    return;

  if (compressed_) {
    conn_->getSender()->append<uint8_t>(out.begin(), out.end());
    // If use mmap() and the mmap file data is compressed, delete the mmap
    // object in the channel
    conn_->getChannel()->removeMMapObject();
  } else if (putinto_buf) {
    conn_->getSender()->append(sv.first, sv.second);
  }
}

HttpResponse::HttpResponse(net::TcpConnection *conn)
    : builder_(new HttpResponseBuilder(
          static_cast<HttpRequest *>(conn->getContext()), conn)) {}

void HttpResponse::sendAuth(HttpAuthType type) { builder_->setAuthType(type); }

void HttpResponse::sendRedirect(const std::string &url) {
  this->setCode(HttpStatus::FOUND).setHeader("Location", url);
}

void HttpResponse::send() {
  if (builder_->connection() == nullptr)
    return;
  builder_->connection()->setKeepAlive(builder_->isKeepAlive());
  builder_->build();
}
