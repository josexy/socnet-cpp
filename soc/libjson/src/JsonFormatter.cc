#include "../include/JsonFormatter.h"

using namespace libjson;

void JsonFormatter::recursive_formatter(int depth, JsonArray *array,
                                        bool empty) {
  __fmt << "[";
  int pos = 0;
  int size = array->__lstValue.size();
  bool once = false;
  for (auto &v : array->__lstValue) {
    if (!once) {
      __fmt << NEWLINE;
      once = true;
    }
    append_space_character(depth * __indent);
    if (v->type() == JsonType::Object) {
      recursive_formatter(depth + 1, v->toJsonObject(), v->empty());
    } else if (v->type() == JsonType::Array) {
      recursive_formatter(depth + 1, v->toJsonArray(), v->empty());
    } else {
      if (v->type() == JsonType::String)
        __fmt << ((JsonString *)(v.get()))->toString(__escape);
      else
        __fmt << v->toString();
    }
    if (pos + 1 < size) {
      __fmt << ",";
    }
    pos++;
    __fmt << NEWLINE;
  }
  // []
  if (!empty)
    append_space_character((depth - 1) * __indent);
  __fmt << "]";
}

void JsonFormatter::recursive_formatter(int depth, JsonObject *object,
                                        bool empty) {
  __fmt << "{";
  int pos = 0;
  int size = object->__kvObjects.size();
  bool once = false;
  for (auto &[k, v] : object->__kvObjects) {
    if (!once) {
      __fmt << NEWLINE;
      once = true;
    }
    // Add indentation when recursing to different depths
    append_space_character(depth * __indent);

    __fmt << k->toString(__escape) << ": ";
    if (v->type() == JsonType::Object) {
      // Recursive formatting Object
      recursive_formatter(depth + 1, v->toJsonObject(), v->empty());
    } else if (v->type() == JsonType::Array) {
      // Recursive formatting Array
      recursive_formatter(depth + 1, v->toJsonArray(), v->empty());
    } else {
      if (v->type() == JsonType::String)
        __fmt << ((JsonString *)(v.get()))->toString(__escape);
      else
        __fmt << v->toString();
    }
    if (pos + 1 < size) {
      __fmt << ",";
    }
    pos++;
    __fmt << NEWLINE;
  }
  // {}
  if (!empty)
    append_space_character((depth - 1) * __indent);
  __fmt << "}";
}