#ifndef LIBJSON_JSONVALUE_H_
#define LIBJSON_JSONVALUE_H_

#include "JsonToken.h"

namespace libjson {

enum class JsonType { Object, Array, String, Number, Boolean, Null };

class JsonString;
class JsonNumber;
class JsonBoolean;
class JsonNull;
class JsonArray;
class JsonObject;

class JsonValue {
public:
  JsonValue(JsonType type) : __type(type) {}

  JsonValue(Token *valueToken) {
    switch (valueToken->tag()) {
    case Tag::JSON_NUMBER:
    case Tag::JSON_FLOAT:
      __type = JsonType::Number;
      break;
    case Tag::JSON_TRUE:
    case Tag::JSON_FALSE:
      __type = JsonType::Boolean;
      break;
    case Tag::JSON_STRING:
      __type = JsonType::String;
      break;
    case Tag::JSON_NULL:
      __type = JsonType::Null;
      break;
    default:
      break;
    }
    __tupleValuePackage = std::make_tuple(
        valueToken->value<std::string>(), valueToken->value<int>(),
        valueToken->value<double>(), valueToken->value<bool>());
  }

  virtual ~JsonValue() {}
  virtual std::string toString() { return ""; };
  virtual JsonType type() { return __type; }

  virtual bool empty() const { return true; }

  virtual JsonNumber *toJsonNumber() { return nullptr; }
  virtual JsonBoolean *toJsonBoolean() { return nullptr; }
  virtual JsonNull *toJsonNull() { return nullptr; }
  virtual JsonString *toJsonString() { return nullptr; }
  virtual JsonArray *toJsonArray() { return nullptr; }
  virtual JsonObject *toJsonObject() { return nullptr; }

  template <class T> T value() {
    return std::get<indexOfValue<T>::index>(__tupleValuePackage);
  }

protected:
  JsonType __type;
  std::tuple<std::string, int, double, bool> __tupleValuePackage;
};

} // namespace libjson
#endif
