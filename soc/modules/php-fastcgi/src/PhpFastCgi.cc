#include "../include/PhpFastCgi.h"

using namespace soc;

PhpFastCgi::PhpFastCgi(int reqid) : requestId_(reqid) {
  params_ = {{FCGI_Params::REQUEST_METHOD, "REQUEST_METHOD"},
             {FCGI_Params::QUERY_STRING, "QUERY_STRING"},
             {FCGI_Params::CONTENT_LENGTH, "CONTENT_LENGTH"},
             {FCGI_Params::CONTENT_TYPE, "CONTENT_TYPE"},
             {FCGI_Params::SCRIPT_FILENAME, "SCRIPT_FILENAME"},
             {FCGI_Params::SERVER_NAME, "SERVER_NAME"},
             {FCGI_Params::SERVER_PORT, "SERVER_PORT"},
             {FCGI_Params::REMOTE_HOST, "REMOTE_ADDR"},
             {FCGI_Params::USER_AGENT, "USER_AGENT"},
             {FCGI_Params::PHP_AUTH_USER, "PHP_AUTH_USER"},
             {FCGI_Params::PHP_AUTH_PW, "PHP_AUTH_PW"},
             {FCGI_Params::PHP_AUTH_DIGEST, "PHP_AUTH_DIGEST"}};
}

PhpFastCgi::~PhpFastCgi() { ::close(sockfd_); }

bool PhpFastCgi::connectPhpFpm(const std::string &ip, uint16_t port) {
  sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in si;
  ::memset(&si, 0, sizeof(si));
  si.sin_addr.s_addr = ::inet_addr(ip.c_str());
  si.sin_family = AF_INET;
  si.sin_port = ::htons(port);

  if (-1 == ::connect(sockfd_, (struct sockaddr *)&si, sizeof(si))) {
    fprintf(stderr, "Connect php-fpm server failed: %s\n", ::strerror(errno));
    return false;
  }
  return true;
}

bool PhpFastCgi::connectPhpFpm(const std::string &sockpath) {
  sockfd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un un;
  ::memset(&un, 0, sizeof(un));
  un.sun_family = AF_UNIX;

  ::strncpy(un.sun_path, sockpath.data(), sockpath.size());

  if (-1 == ::connect(sockfd_, (struct sockaddr *)&un, sizeof(un))) {
    fprintf(stderr, "Connect php-fpm server failed: %s\n", ::strerror(errno));
    return false;
  }
  return true;
}

FCGI_Header PhpFastCgi::makeHeader(int type, int request, int contentlen,
                                   int paddinglen) {
  FCGI_Header header;
  header.version = FCGI_VERSION_1;
  header.type = (unsigned char)type;
  /* 两个字段保存请求ID */
  header.requestIdB1 = (unsigned char)((requestId_ >> 8) & 0xff);
  header.requestIdB0 = (unsigned char)(requestId_ & 0xff);
  /* 两个字段保存内容长度 */
  header.contentLengthB1 = (unsigned char)((contentlen >> 8) & 0xff);
  header.contentLengthB0 = (unsigned char)(contentlen & 0xff);
  /* 填充字节的长度 */
  header.paddingLength = (unsigned char)paddinglen;
  /* 保存字节赋为 0 */
  header.reserved = 0;
  return header;
}

FCGI_BeginRequestBody PhpFastCgi::makeBeginRequestBody(int role, int keepconn) {
  FCGI_BeginRequestBody body;
  /* 两个字节保存期望 php-fpm 扮演的角色 */
  body.roleB1 = (unsigned char)((role >> 8) & 0xff);
  body.roleB0 = (unsigned char)(role & 0xff);
  /* 大于0长连接，否则短连接 */
  body.flags = (unsigned char)((keepconn) ? FCGI_KEEP_CONN : 0);
  ::bzero(&body.reserved, sizeof(body.reserved));
  return body;
}

void PhpFastCgi::makeNameValueBody(const std::string &name,
                                   const std::string &value,
                                   unsigned char *body, int *bodylen) {
  /* 记录 body 的开始位置 */
  unsigned char *startBodyBuffPtr = body;
  size_t nlen = name.size();
  size_t vlen = value.size();
  /* 如果 nameLen 小于128字节 */
  if (nlen < 128) {
    *body++ = (unsigned char)nlen; // nameLen用1个字节保存
  } else {
    /* nameLen 用 4 个字节保存 */
    *body++ = (unsigned char)((nlen >> 24) | 0x80);
    *body++ = (unsigned char)(nlen >> 16);
    *body++ = (unsigned char)(nlen >> 8);
    *body++ = (unsigned char)nlen;
  }

  /* valueLen 小于 128 就用一个字节保存 */
  if (vlen < 128) {
    *body++ = (unsigned char)vlen;
  } else {
    /* valueLen 用 4 个字节保存 */
    *body++ = (unsigned char)((vlen >> 24) | 0x80);
    *body++ = (unsigned char)(vlen >> 16);
    *body++ = (unsigned char)(vlen >> 8);
    *body++ = (unsigned char)vlen;
  }

  /* 将 name 中的字节逐一加入body中的buffer中 */
  for (int i = 0; i < nlen; i++) {
    *body++ = name[i];
  }

  /* 将 value 中的值逐一加入body中的buffer中 */
  for (int i = 0; i < vlen; i++) {
    *body++ = value[i];
  }

  /* 计算出 body 的长度 */
  *bodylen = body - startBodyBuffPtr;
}

void PhpFastCgi::sendStartRequestRecord() {
  FCGI_BeginRequestRecord beginRecord;
  beginRecord.header =
      makeHeader(FCGI_BEGIN_REQUEST, requestId_, sizeof(beginRecord.body), 0);
  beginRecord.body = makeBeginRequestBody(FCGI_RESPONDER, 0);

  ::write(sockfd_, (char *)&beginRecord, sizeof(beginRecord));
}

void PhpFastCgi::sendPost(const std::string_view &postdata) {
  size_t size = postdata.size();

  FCGI_Header startHeader = makeHeader(FCGI_STDIN, requestId_, size, 0);
  ::write(sockfd_, &startHeader, FCGI_HEADER_LEN);

  // POST data
  int rn = size;
  int n = -1;
  while ((n = ::write(sockfd_, postdata.data() + size - rn, rn)) > 0) {
    rn -= n;
  }

  FCGI_Header endHeader = makeHeader(FCGI_STDIN, requestId_, 0, 0);
  ::write(sockfd_, &endHeader, FCGI_HEADER_LEN);
}

void PhpFastCgi::sendParams(FCGI_Params param, const std::string &value) {
  int rc;
  unsigned char bodyBuff[kBufferSize];
  ::bzero(bodyBuff, sizeof(bodyBuff));

  /* 保存 body 的长度 */
  int bodyLen;

  /* 生成 PARAMS 参数内容的 body */
  makeNameValueBody(params_[param], value, bodyBuff, &bodyLen);

  FCGI_Header nameValueHeader = makeHeader(FCGI_PARAMS, requestId_, bodyLen, 0);

  int nameValueRecordLen = bodyLen + FCGI_HEADER_LEN;
  char nameValueRecord[nameValueRecordLen];

  /* 将header和body拷贝到一块buffer中只需调用一次write */
  ::memcpy(nameValueRecord, (char *)&nameValueHeader, FCGI_HEADER_LEN);
  ::memcpy(nameValueRecord + FCGI_HEADER_LEN, bodyBuff, bodyLen);

  ::write(sockfd_, nameValueRecord, nameValueRecordLen);
}

void PhpFastCgi::sendEndRequestRecord() {
  FCGI_Header endHeader = makeHeader(FCGI_PARAMS, requestId_, 0, 0);
  ::write(sockfd_, (char *)&endHeader, FCGI_HEADER_LEN);
}

std::pair<bool, std::string> PhpFastCgi::readPhpFpm(net::Buffer *buffer) {
  FCGI_Header responderHeader;
  char content[kBufferSize]{0};
  int contentLen;
  char tmp[8]; //用来暂存padding字节的
  int ret;

  bool status = true;
  std::string message = "OK";
  // 读取Header
  while (::read(sockfd_, &responderHeader, FCGI_HEADER_LEN) > 0) {
    // PHP-FPM正常返回输出
    if (responderHeader.type == FCGI_STDOUT) {
      //获取内容长度
      contentLen = (responderHeader.contentLengthB1 << 8) +
                   (responderHeader.contentLengthB0);

      int nbuf = contentLen / kBufferSize;
      int remain = contentLen % kBufferSize;
      while (nbuf) {
        ret = ::read(sockfd_, content, kBufferSize);
        buffer->append(content, ret);
        nbuf--;
      }
      ret = ::read(sockfd_, content, remain);
      buffer->append(content, ret);

      //跳过填充部分
      if (responderHeader.paddingLength > 0) {
        ret = ::read(sockfd_, tmp, responderHeader.paddingLength);
      }
    } else if (responderHeader.type == FCGI_STDERR) {
      // PHP-FPM返回异常错误
      contentLen = (responderHeader.contentLengthB1 << 8) +
                   (responderHeader.contentLengthB0);

      ret = ::read(sockfd_, content, contentLen);
      status = false;
      message = std::string(content, ret);
      //跳过填充部分
      if (responderHeader.paddingLength > 0) {
        ret = ::read(sockfd_, tmp, responderHeader.paddingLength);
      }
    } else if (responderHeader.type == FCGI_END_REQUEST) {
      FCGI_EndRequestBody endRequest;
      ret = ::read(sockfd_, &endRequest, sizeof(endRequest));
    }
  }
  return std::make_pair(status, message);
}