#ifndef SOC_MODULE_PHPFASTCGI_H
#define SOC_MODULE_PHPFASTCGI_H

#include "../../../net/include/Buffer.h"
#include "FastCgi.h"
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <string>
#include <unordered_map>

namespace soc {

constexpr static const size_t kBufferSize = 1024;

enum class FCGI_Params {
  REQUEST_METHOD,
  QUERY_STRING,
  CONTENT_LENGTH,
  CONTENT_TYPE,
  SCRIPT_FILENAME,
  SERVER_NAME,
  SERVER_PORT,
  REMOTE_ADDR,
  REMOTE_HOST,
  AUTORIZATION,
  USER_AGENT,

  PHP_AUTH_USER,
  PHP_AUTH_PW,
  PHP_AUTH_DIGEST
};

namespace net {
class Buffer;
}

class PhpFastCgi {
public:
  explicit PhpFastCgi(int reqid = 1);
  ~PhpFastCgi();

  bool connectPhpFpm(const std::string &ip, uint16_t port);
  bool connectPhpFpm(const std::string &sockpath);

  FCGI_Header makeHeader(int type, int request, int contentlen, int paddinglen);
  FCGI_BeginRequestBody makeBeginRequestBody(int role, int keepconn);
  void makeNameValueBody(const std::string &name, const std::string &value,
                         unsigned char *body, int *bodylen);
  void sendStartRequestRecord();
  void sendParams(FCGI_Params name, const std::string &value);
  void sendEndRequestRecord();

  void sendPost(const std::string_view &postdata);

  std::pair<bool, std::string> readPhpFpm(net::Buffer *buffer);

  int fd() const { return sockfd_; }
  int reqId() const { return requestId_; }

private:
  std::unordered_map<FCGI_Params, std::string> params_;
  int sockfd_;
  int requestId_;
};

} // namespace soc

#endif
