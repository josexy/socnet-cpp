#ifndef LIBJSON_JSONTOKEN_H
#define LIBJSON_JSONTOKEN_H

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

namespace libjson {

enum class Tag {
  JSON_STRING, // string
  JSON_NUMBER, // integer
  JSON_FLOAT,  // float
  JSON_TRUE,   // true
  JSON_FALSE,  // false
  JSON_NULL,   // null
  JSON_OBJECT_BEGIN = '{',
  JSON_OBJECT_END = '}',
  JSON_ARRAY_BEGIN = '[',
  JSON_ARRAY_END = ']',
  JSON_KEY_VALUE_SEP = ':',
  JSON_SEP = ','
};

//
template <class> struct indexOfValue {};
template <> struct indexOfValue<std::string> {
  static constexpr int index = 0;
};
template <> struct indexOfValue<int> { static constexpr int index = 1; };
template <> struct indexOfValue<double> { static constexpr int index = 2; };
template <> struct indexOfValue<bool> { static constexpr int index = 3; };

class Token {
public:
  Token(Tag tag) : Token(tag, 1, 1, 0) {}

  Token(Tag tag, int line, int col, int len)
      : __tag(tag), __line(line), __col(col), __len(len) {}

  virtual ~Token() {}

  // token type
  virtual Tag tag() const { return __tag; }

  // line number of token
  virtual int line() const { return __line; }
  // column number of token
  virtual int col() const { return __col; }
  // length of token
  virtual int len() const { return __len; }

  virtual std::string toString() {
    std::stringstream ss;
    if (__tag == Tag::JSON_STRING)
      ss << "<String, \"" << value<std::string>() << "\">";
    else if (__tag == Tag::JSON_NUMBER)
      ss << "<Number, " << value<int>() << ">";
    else if (__tag == Tag::JSON_FLOAT)
      ss << "<Float, " << value<double>() << ">";
    else if (__tag == Tag::JSON_TRUE)
      ss << "<True>";
    else if (__tag == Tag::JSON_FALSE)
      ss << "<False>";
    else if (__tag == Tag::JSON_NULL)
      ss << "<Null>";
    else
      ss << "<" << static_cast<char>(__tag) << ">";
    ss << " [" << __line << "," << __col << "," << __len << "]";
    return ss.str();
  }

  friend std::ostream &operator<<(std::ostream &out, Token *token) {
    out << token->toString();
    return out;
  }

  // Get a element of the specified type from the tuple
  template <class T> T value() {
    return std::get<indexOfValue<T>::index>(__tupleValuePackage);
  }

protected:
  Tag __tag;
  int __line, __col, __len;
  std::tuple<std::string, int, double, bool> __tupleValuePackage;
};

// identifier: null/true/false
class Identifier : public Token {
public:
  Identifier(Tag tag, const std::string &identifier, int line, int col, int len)
      : Token(tag, line, col, len) {
    __tupleValuePackage =
        std::make_tuple(identifier, 0, 0.0, tag == Tag::JSON_TRUE);
  }
};

// integer
class Number : public Token {
public:
  Number(int value, int line, int col, int len)
      : Token(Tag::JSON_NUMBER, line, col, len) {
    __tupleValuePackage = std::make_tuple("", value, 0.0, false);
  }
};

// float
class Float : public Token {
public:
  Float(double value, int line, int col, int len)
      : Token(Tag::JSON_FLOAT, line, col, len) {
    __tupleValuePackage = std::make_tuple("", 0, value, false);
  }
};

// string literal
class String : public Token {
public:
  String(const std::string &lexeme, int line, int col, int len)
      : Token(Tag::JSON_STRING, line, col, len) {
    __tupleValuePackage = std::make_tuple(lexeme, 0, 0.0, false);
  }
};
} // namespace libjson
#endif
