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
struct ObjNative;
struct ObjFunction;
struct ObjClosure;
struct ObjClass;
struct ObjInstance;

using Nil = std::monostate;
using String = std::string;
using Function = std::shared_ptr<ObjFunction>;
using NativeFunction = std::shared_ptr<ObjNative>;
using Closure = std::shared_ptr<ObjClosure>;
using Klass = std::shared_ptr<ObjClass>;
using Instance = std::shared_ptr<ObjInstance>;

enum ValueType
{
  VAL_NIL,
  VAL_BOOL,
  VAL_NUMBER,
  VAL_STRING,
  VAL_FUNCTION,
  VAL_CLOSURE,
  VAL_NATIVE,
  VAL_CLASS,
  VAL_INSTANCE,
};
struct Value
{
  ValueType type;

  using value_t = std::variant<bool, double, Nil, String, Function, NativeFunction, Closure, Klass, Instance>;
  value_t as;

  template <class T>
  T get()
  {
    T val;

    try
    {
      val = std::get<T>(as); // w contains int, not float: will throw
    }
    catch (const std::bad_variant_access &ex)
    {

      std::cout << "Execption :" << ex.what() << " : " << typeid(T).name() << '\n';
    }

    return val;
  }

  bool operator==(const Value &rhs)
  {
    return as == rhs.as;
  }
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative
{
  NativeFn function;
  ObjNative(NativeFn native);
};

struct ObjUpvalue
{
  Value *location;
  Value closed;
  struct ObjUpvalue *next;
  ObjUpvalue(Value *slot);
};

struct ObjFunction
{
  int arity;
  Chunk *chunk;
  std::string name;
  int upvalueCount;
  ObjFunction();
  ~ObjFunction();
};

enum FunctionType
{
  TYPE_FUNCTION,
  TYPE_SCRIPT
};

struct ObjClosure
{
  Function function;
  ObjUpvalue **upvalues;
  int upvalueCount;
  ObjClosure(Function fn);
  ~ObjClosure();
};

struct ObjClass
{
  std::string name;
  ObjClass(std::string name);
};

using StringMap = std::unordered_map<String, Value>;
struct ObjInstance {
  Klass klass;
  StringMap fields; 
  ObjInstance(Klass k);
} ;

// custom specialization of std::hash can be injected in namespace std
struct ValueHash
{
  std::size_t operator()(Value const &v) const noexcept
  {
    std::size_t h1 = std::hash<Value::value_t>{}(v.as);
    return h1;
  }
};

// using Entry = std::pair<std::string, Value>;

// using Table = std::unordered_map<Value, Value, ValueHash>;

#define BOOL_VAL(value) \
  Value { VAL_BOOL, value }
#define NIL_VAL \
  Value         \
  {             \
    VAL_NIL, {} \
  }
#define NUMBER_VAL(value) \
  Value { VAL_NUMBER, value }
#define STRING_VAL(value) \
  Value { VAL_STRING, value }
#define FUNCTION_VAL(value) \
  Value { VAL_FUNCTION, value }
#define NATIVE_VAL(value) \
  Value { VAL_NATIVE, value }
#define CLOSURE_VAL(value) \
  Value { VAL_CLOSURE, value }
#define CLASS_VAL(value) \
  Value { VAL_CLASS, value }
#define INSTANCE_VAL(value) \
  Value { VAL_INSTANCE, value }


#define IS_BOOL(value) (value.type == VAL_BOOL)
#define IS_NIL(value) (value.type == VAL_NIL)
#define IS_NUMBER(value) (value.type == VAL_NUMBER)
#define IS_STRING(value) (value.type == VAL_STRING)
#define IS_FUNCTION(value) (value.type == VAL_FUNCTION)
#define IS_NATIVE(value) (value.type == VAL_NATIVE)
#define IS_CLOSURE(value) (value.type == VAL_CLOSURE)
#define IS_CLASS(value) (value.type == VAL_CLASS)
#define IS_INSTANCE(value) (value.type == VAL_INSTANCE)

#define AS_BOOL(value) (value.get<bool>())
#define AS_NUMBER(value) (value.get<double>())
#define AS_STRING(value) (value.get<std::string>())
#define AS_CSTRING(value) (value.get<std::string>().c_str())
#define AS_FUNCTION(value) (value.get<Function>())
#define AS_NATIVEFN(value) (value.get<NativeFunction>()->function)
#define AS_CLOSURE(value) (value.get<Closure>())
#define AS_CLASS(value) (value.get <Klass>())
#define AS_INSTANCE(value) (value.get <Instance>())

struct OutputVisitor
{
  void operator()(const double d) const { std::cout << d; }
  void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
  void operator()(const std::monostate n) const { std::cout << "nil"; }
  void operator()(const std::string &s) const { std::cout << s; }
  void operator()(const NativeFunction &n) const { std::cout << "<native fn>"; }
  void operator()(const Closure &c) const
  {
    operator()(c->function);
  }
  void operator()(const Function &f) const
  {
    if (f->name != "")
      std::cout << "<fn " << f->name << ">";
    else
      std::cout << "<srcipt>";
  }
  void operator()(const Klass &k) const { std::cout << k->name; }
  void operator()(const Instance &i) const { std::cout << i->klass->name << " instance"; }
};

std::ostream &operator<<(std::ostream &os, const Value &v);
void printValue(Value value);

bool isFalsey(Value value);

bool valuesEqual(Value a, Value b);

std::string copyString(const char *chars, int length);