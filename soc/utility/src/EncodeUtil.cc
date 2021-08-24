#include "../include/EncodeUtil.h"

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <string.h>
#include <zlib.h>

using namespace soc;

void EncodeUtil::gzipCompress(std::string_view buf,
                              std::vector<uint8_t> &output) {
  size_t size = buf.size();
  z_stream strm;
  ::memset(&strm, 0, sizeof(strm));

  deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8,
               Z_DEFAULT_STRATEGY);

  strm.next_in = reinterpret_cast<z_const Bytef *>((void *)buf.data());
  strm.avail_in = static_cast<uint32_t>(size);

  std::size_t size_compressed = 0;
  do {
    size_t increase = size / 2 + 1024;
    if (output.size() < (size_compressed + increase)) {
      output.resize(size_compressed + increase);
    }
    strm.avail_out = static_cast<unsigned int>(increase);
    strm.next_out =
        reinterpret_cast<Bytef *>((output.data() + size_compressed));

    ::deflate(&strm, Z_FINISH);
    size_compressed += (increase - strm.avail_out);
  } while (strm.avail_out == 0);

  ::deflateEnd(&strm);
  output.resize(size_compressed);
}

unsigned char *EncodeUtil::md5Hash(void *data, size_t n, unsigned char md[16]) {
  return ::MD5(reinterpret_cast<const unsigned char *>(data), n, md);
}

std::string EncodeUtil::md5Hash(std::string_view data) {
  unsigned char md[16];
  ::MD5(reinterpret_cast<const unsigned char *>(data.data()), data.size(), md);
  std::string v(32, 0);
  for (int i = 0, j = 0; i < 16 && j < 32; ++i, j += 2)
    ::sprintf(v.data() + j, "%02x", md[i]);
  return v;
}

bool EncodeUtil::md5HashEqual(std::string_view data, std::string_view hashv) {
  std::string hashv2 = md5Hash(data);
  return 0 == ::memcmp(hashv2.data(), hashv.data(), hashv2.size());
}

std::string EncodeUtil::md5HashFile(const char *filename) {
  FILE *fp = NULL;
  fp = ::fopen(filename, "rb");
  if (fp == NULL)
    return "";

  MD5_CTX ctx;
  unsigned char md[16];
  char buffer[1024];
  int n;
  ::MD5_Init(&ctx);
  while ((n = ::fread(buffer, sizeof(char), 1024, fp)) > 0) {
    ::MD5_Update(&ctx, buffer, n);
    ::memset(buffer, 0, sizeof(buffer));
  }
  ::MD5_Final(md, &ctx);
  ::fclose(fp);

  std::string v(32, 0);
  for (int i = 0, j = 0; i < 16 && j < 32; ++i, j += 2)
    sprintf(v.data() + j, "%02x", md[i]);
  return v;
}

unsigned int EncodeUtil::murmurHash2(std::string_view s) {
  constexpr unsigned int m = 0x5bd1e995;
  constexpr int r = 24;
  // Initialize the hash to a 'random' value
  size_t len = s.size();
  unsigned int h = 0xEE6B27EB ^ len;

  // Mix 4 bytes at a time into the hash

  const unsigned char *data = (const unsigned char *)s.data();

  while (len >= 4) {
    unsigned int k = *(unsigned int *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array
  switch (len) {
  case 3:
    h ^= data[2] << 16;
  case 2:
    h ^= data[1] << 8;
  case 1:
    h ^= data[0];
    h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  return h;
}

std::string EncodeUtil::base64Encode(std::string_view data) {
  return boost::beast::detail::base64_encode(
      reinterpret_cast<std::uint8_t const *>(data.data()), data.size());
}

std::string EncodeUtil::base64Decode(std::string_view data) {
  return boost::beast::detail::base64_decode(
      std::string(data.data(), data.size()));
}

unsigned char EncodeUtil::hex2dec(char c) {
  if (c >= '0' && c <= '9')
    return c - '0'; // [0-9]
  else if (c >= 'a' && c <= 'z')
    return c - 'a' + 10; // a-f [10-15]
  else if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10; // A-F [10-15]
  return c;
}

char EncodeUtil::dec2hex(char c) {
  if (c >= 0 && c <= 9)
    return c + '0'; // ['0'-'9']
  else if (c >= 10 && c <= 15)
    return c - 10 + 'A'; // ['A'-'Z']
  return c;
}

std::string EncodeUtil::urlDecode(const char *value, size_t size) {
  std::string escaped;
  for (size_t i = 0; i < size; ++i) {
    if (value[i] == '%' && i + 2 < size && isxdigit(value[i + 1]) &&
        isxdigit(value[i + 2])) {
      // merge two char to one byte
      // %AB => 0xAB
      unsigned char byte =
          ((unsigned char)hex2dec(value[i + 1]) << 4) | hex2dec(value[i + 2]);
      i += 2;
      escaped += byte;
    } else {
      escaped += value[i];
    }
  }
  return escaped;
}

std::string EncodeUtil::urlDecode(std::string_view value) {
  return urlDecode(value.data(), value.size());
}

std::string EncodeUtil::urlDecode(const char *value) {
  return urlDecode(value, strlen(value));
}

std::string EncodeUtil::urlEncode(const char *value, size_t size) {
  std::string escaped;

  for (size_t i = 0; i < size; i++) {
    unsigned char c = (unsigned char)value[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped += c;
      continue;
    }
    // split one byte to two char
    // 0xAB => %AB
    char buf[5]{0};
    sprintf(buf, "%%%c%c", toupper(dec2hex(c >> 4)), toupper(dec2hex(c & 15)));
    escaped += buf;
  }

  return escaped;
}

std::string EncodeUtil::urlEncode(std::string_view value) {
  return urlEncode(value.data(), value.size());
}

std::string EncodeUtil::urlEncode(const char *value) {
  return urlEncode(value, strlen(value));
}

void EncodeUtil::toLowers(char *s, size_t size) {
  for (size_t i = 0; i < size; i++)
    if (isupper(s[i]))
      s[i] = tolower(s[i]);
}

void EncodeUtil::toUppers(char *s, size_t size) {
  for (size_t i = 0; i < size; i++)
    if (islower(s[i]))
      s[i] = toupper(s[i]);
}

std::string EncodeUtil::toLowers(const std::string &s) {
  std::string ls(s);
  toLowers(ls.data(), ls.size());
  return ls;
}

std::string EncodeUtil::toUppers(const std::string &s) {
  std::string us(s);
  toUppers(us.data(), us.size());
  return us;
}