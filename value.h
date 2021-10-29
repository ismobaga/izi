#pragma once


#include "common.h"
#include <variant>
#include <iostream>
#include <string>
#include <iterator>
#include <unordered_map>
#include <functional>







 enum ValueType {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_STRING,
} ;



struct Value {
  ValueType type;
    
  using value_t = std::variant<bool, double, std::monostate, std::string>  ;
  value_t as; 

} ;
// custom specialization of std::hash can be injected in namespace std
namespace std
{
    template<> struct hash<Value>
    {
        std::size_t operator()(Value const& v) const noexcept
        {
            std::size_t h1 = std::hash<Value::value_t>{}(v.as);
            // std::size_t h2 = std::hash<std::string>{}(s.last_name);
            return h1;// ^ (h2 << 1); // or use boost::hash_combine
        }
    };
}
 

// using Entry = std::pair<std::string, Value>;
using Table = std::unordered_map<Value, Value>;




#define BOOL_VAL(value)   Value{VAL_BOOL,  value}
#define NIL_VAL           Value{VAL_NIL,  {}}
#define NUMBER_VAL(value) Value{VAL_NUMBER,  value}
#define STRING_VAL(value)  Value{VAL_STRING, value}

#define IS_BOOL(value)    (value.type == VAL_BOOL)
#define IS_NIL(value)     (value.type == VAL_NIL)
#define IS_NUMBER(value)  (value.type == VAL_NUMBER)
#define IS_STRING(value)  (value.type == VAL_STRING)

#define AS_BOOL(value)    (std::get<bool>(value.as))
#define AS_NUMBER(value)  (std::get<double>(value.as))
#define AS_STRING(value)  (std::get<std::string>(value.as))
#define AS_CSTRING(value)  (std::get<std::string>(value.as).c_str())



struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate n) const { std::cout << "nil"; }
    void operator()(const std::string& s) const { std::cout << s; }
};

std::ostream& operator<<(std::ostream& os, const Value& v);
void printValue(Value value);

bool isFalsey(Value value);

bool valuesEqual(Value a, Value b);

std::string copyString(const char* chars, int length);