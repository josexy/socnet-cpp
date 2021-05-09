#ifndef LIBJSON_JSONBOOLEAN_H
#define LIBJSON_JSONBOOLEAN_H

#include "JsonValue.h"

namespace libjson {

class JsonBoolean : public JsonValue {
public:
  JsonBoolean(bool value) : JsonValue(JsonType::Boolean) {
    __tupleValuePackage = std::make_tuple("", 0, 0.0, value);
  }

  std::string toString() { return value<bool>() ? "true" : "false"; }
  JsonBoolean *toJsonBoolean() { return this; }
  ~JsonBoolean() {}
};
} // namespace libjson
#endif
