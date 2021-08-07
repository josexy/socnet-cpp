#ifndef SOC_UTILITY_ENCODEUTIL_H
#define SOC_UTILITY_ENCODEUTIL_H

#include "../../modules/base64/include/Base64.h"
#include <functional>

namespace soc {
namespace net {
class Buffer;
}

struct EncodeUtil {
  // Gzip compress algorithm for http response body
  static void gzipCompress(std::string_view, std::vector<uint8_t> &);

  // MD5
  static unsigned char *md5Hash(void *, size_t, unsigned char[16]);
  static std::string md5Hash(std::string_view);
  static bool md5HashEqual(std::string_view, std::string_view);
  static std::string md5HashFile(const char *);

  // MurmurHash2, by Austin Appleby
  static unsigned int murmurHash2(std::string_view);

  // Base64 encode/decode
  static std::string base64Encode(std::string_view);
  static std::string base64Decode(std::string_view);

  // Url encode/decode
  static unsigned char hex2dec(char);
  static char dec2hex(char);

  static std::string urlDecode(const char *, size_t);
  static std::string urlDecode(std::string_view);
  static std::string urlDecode(const char *);

  static std::string urlEncode(const char *, size_t);
  static std::string urlEncode(std::string_view);
  static std::string urlEncode(const char *);

  static void toLowers(char *, size_t);
  static void toUppers(char *, size_t);

  static std::string genRandromStr(size_t);
};
} // namespace soc

#endif
