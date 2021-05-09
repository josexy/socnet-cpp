#ifndef LIBJSON_JSONNULL_H
#define LIBJSON_JSONNULL_H

#include "JsonValue.h"

namespace libjson {
class JsonNull : public JsonValue {
public:
  JsonNull() : JsonValue(JsonType::Null) {}
  std::string toString() { return "null"; }

  JsonNull *toJsonNull() { return this; }
  ~JsonNull() {}
};
} // namespace libjson
#endif
