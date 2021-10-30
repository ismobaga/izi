#pragma once


#include "common.h"
#include <variant>
#include <iostream>
#include <string>
#include <iterator>
#include <unordered_map>
#include <functional>
#include <memory>


class Chunk;


struct ObjFunction {
  int arity;
  Chunk *chunk;
  std::string name;
  ObjFunction();
  ~ObjFunction();

} ;

enum FunctionType {
  TYPE_FUNCTION,
  TYPE_SCRIPT
} ;

using Function = std::shared_ptr<ObjFunction>;


 enum ValueType {
  VAL_BOOL,
  VAL_NIL, 
  VAL_NUMBER,
  VAL_STRING,
  VAL_FUNCTION
} ;



struct Value {
  ValueType type;
    
  using value_t = std::variant<bool, double, std::monostate, std::string, Function>  ;
  value_t as; 

  bool operator==(const Value& rhs) {
    return as == rhs.as;
}


} ;
// custom specialization of std::hash can be injected in namespace std
struct ValueHash
    {
        std::size_t operator()(Value const& v) const noexcept
        {
            std::size_t h1 = std::hash<Value::value_t>{}(v.as);
            return h1;
        }
    };


// using Entry = std::pair<std::string, Value>;
using Table = std::unordered_map<Value, Value, ValueHash>;




#define BOOL_VAL(value)   Value{VAL_BOOL,  value}
#define NIL_VAL           Value{VAL_NIL,  {}}
#define NUMBER_VAL(value) Value{VAL_NUMBER,  value}
#define STRING_VAL(value)  Value{VAL_STRING, value}
#define FUNCTION_VAL(value)  Value{VAL_FUNCTION, value}

#define IS_BOOL(value)    (value.type == VAL_BOOL)
#define IS_NIL(value)     (value.type == VAL_NIL)
#define IS_NUMBER(value)  (value.type == VAL_NUMBER)
#define IS_STRING(value)  (value.type == VAL_STRING)
#define IS_FUNCTION(value)  (value.type == VAL_FUNCTION)

#define AS_BOOL(value)    (std::get<bool>(value.as))
#define AS_NUMBER(value)  (std::get<double>(value.as))
#define AS_STRING(value)  (std::get<std::string>(value.as))
#define AS_CSTRING(value)  (std::get<std::string>(value.as).c_str())
#define AS_FUNCTION(value)  (std::get<Function>(value.as))



struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate n) const { std::cout << "nil"; }
    void operator()(const std::string& s) const { std::cout << s; }
    void operator()(const Function& f) const {if (f->name!="") std::cout << "<fn " << f->name<< ">"; else std::cout<< "<srcipt>"; }
};

std::ostream& operator<<(std::ostream& os, const Value& v);
void printValue(Value value);

bool isFalsey(Value value);

bool valuesEqual(Value a, Value b);

std::string copyString(const char* chars, int length);