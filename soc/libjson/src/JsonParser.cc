#include "../include/JsonParser.h"

using namespace libjson;

void JsonParser::parseJsonObject(std::shared_ptr<JsonObject> &object) {
  while (__tokenIndex < __tokenLength) {
    auto key = lexer.token_list[__tokenIndex++];
    // }
    if (key->tag() == Tag::JSON_OBJECT_END) {
      handle_after_object_array_closure_exception(key.get());
      break;
    }
    // ,
    if (key->tag() == Tag::JSON_SEP) {
      handle_after_separators_exception();
      continue;
    }
    handle_key_not_string_exception(key.get());

    std::shared_ptr<JsonString> key_ptr =
        make_shared<JsonString>(key->value<std::string>(), lexer.escape);

    handle_object_missing_separator_exception(key.get());

    // skip :
    __tokenIndex++;

    handle_object_missing_value();

    auto value = lexer.token_list[__tokenIndex++];
    handle_missing_separator_exception(value.get());

    std::shared_ptr<JsonValue> value_ptr = nullptr;
    switch (value->tag()) {
    case Tag::JSON_OBJECT_BEGIN: { // {
      handle_after_object_array_exception(value.get());
      std::shared_ptr<JsonObject> object_ptr = make_shared<JsonObject>();
      parseJsonObject(object_ptr);
      value_ptr = object_ptr;
    } break;
    case Tag::JSON_ARRAY_BEGIN: { // [
      handle_after_object_array_exception(value.get());
      std::shared_ptr<JsonArray> array_ptr = make_shared<JsonArray>();
      parseJsonArray(array_ptr);
      value_ptr = array_ptr;
    } break;
    case Tag::JSON_NUMBER:
      value_ptr = make_shared<JsonNumber>(value->value<int>(), true);
      break;
    case Tag::JSON_FLOAT:
      value_ptr = make_shared<JsonNumber>(value->value<double>(), false);
      break;
    case Tag::JSON_STRING:
      value_ptr =
          make_shared<JsonString>(value->value<std::string>(), lexer.escape);
      break;
    case Tag::JSON_NULL:
      value_ptr = make_shared<JsonNull>();
      break;
    case Tag::JSON_TRUE:
    case Tag::JSON_FALSE:
      value_ptr = make_shared<JsonBoolean>(value->value<bool>());
      break;
    default:
      break;
    }
    object->add(key_ptr, value_ptr);
  }
}

void JsonParser::parseJsonArray(std::shared_ptr<JsonArray> &array) {
  while (__tokenIndex < __tokenLength) {
    auto value = lexer.token_list[__tokenIndex++];
    // ]
    if (value->tag() == Tag::JSON_ARRAY_END) {
      handle_after_object_array_closure_exception(value.get());
      break;
    }
    // ,
    if (value->tag() == Tag::JSON_SEP) {
      handle_after_separators_exception();
      continue;
    }
    handle_missing_separator_exception(value.get());

    std::shared_ptr<JsonValue> value_ptr = nullptr;
    switch (value->tag()) {
    case Tag::JSON_OBJECT_BEGIN: { // {
      handle_after_object_array_exception(value.get());
      std::shared_ptr<JsonObject> object_ptr = make_shared<JsonObject>();
      parseJsonObject(object_ptr);
      value_ptr = object_ptr;
    } break;
    case Tag::JSON_ARRAY_BEGIN: { // [
      handle_after_object_array_exception(value.get());
      std::shared_ptr<JsonArray> array_ptr = make_shared<JsonArray>();
      parseJsonArray(array_ptr);
      value_ptr = array_ptr;
    } break;
    case Tag::JSON_NUMBER:
      value_ptr = make_shared<JsonNumber>(value->value<int>(), true);
      break;
    case Tag::JSON_FLOAT:
      value_ptr = make_shared<JsonNumber>(value->value<double>(), false);
      break;
    case Tag::JSON_STRING:
      value_ptr =
          make_shared<JsonString>(value->value<std::string>(), lexer.escape);
      break;
    case Tag::JSON_NULL:
      value_ptr = make_shared<JsonNull>();
      break;
    case Tag::JSON_TRUE:
    case Tag::JSON_FALSE:
      value_ptr = make_shared<JsonBoolean>(value->value<bool>());
      break;
    default:
      break;
    }
    array->add(value_ptr);
  }
}