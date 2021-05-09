#ifndef LIBJSON_JSONSTRING_H
#define LIBJSON_JSONSTRING_H

#include "JsonError.h"
#include "JsonUtil.h"
#include "JsonValue.h"
namespace libjson {
class JsonString : public JsonValue {
public:
  JsonString(const std::string &value, bool isEscaped = true)
      : JsonValue(JsonType::String), isEscaped(isEscaped) {
    __tupleValuePackage = std::make_tuple(value, 0, 0.0, false);
  }
  std::string toString() { return "\"" + value<std::string>() + "\""; }

  std::string toString(bool escape) {
    std::string ss = value<std::string>();
    if (escape) {
      if (!isEscaped)
        ss = escape_string(ss);
    } else {
      if (isEscaped)
        ss = unescape_string(ss);
    }
    return "\"" + ss + "\"";
  }
  JsonString *toJsonString() { return this; }

  ~JsonString() {}

private:
  bool isEscaped;
};
} // namespace libjson
#endif
