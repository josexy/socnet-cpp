#ifndef LIBJSON_JSONNUMBER_H
#define LIBJSON_JSONNUMBER_H

#include "JsonUtil.h"
#include "JsonValue.h"

namespace libjson {
class JsonNumber : public JsonValue {
public:
  JsonNumber(double val, bool integer = true)
      : JsonValue(JsonType::Number), integer(integer), val(val) {
    __tupleValuePackage = std::make_tuple("", integer ? (int)(val) : 0,
                                          integer ? 0.0 : val, false);
  }

  std::string toString() {
    std::string s;
    return integer ? toStr<int>(int(val)) : toStr<double>(val);
  }
  JsonNumber *toJsonNumber() { return this; }
  ~JsonNumber() {}

private:
  bool integer;
  double val;
};
} // namespace libjson
#endif