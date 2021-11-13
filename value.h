#pragma once

#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include "common.h"

class Chunk;
struct ObjNative;
struct ObjFunction;
struct ObjClosure;
struct ObjClass;
struct ObjInstance;
struct ObjBoundMethod;
struct ObjModule;

using Nil = std::monostate;
using String = std::string;
using Function = std::shared_ptr<ObjFunction>;
using NativeFunction = std::shared_ptr<ObjNative>;
using Closure = std::shared_ptr<ObjClosure>;
using Klass = std::shared_ptr<ObjClass>;
using Instance = std::shared_ptr<ObjInstance>;
using BoundMethod = std::shared_ptr<ObjBoundMethod>;
using Module = std::shared_ptr<ObjModule>;

enum ValueType {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_CLOSURE,
    VAL_NATIVE,
    VAL_CLASS,
    VAL_INSTANCE,
    VAL_BOUND_METHOD,
    VAL_MODULE,
};

/**
 * @brief IZI Value type
 * 
 */
struct Value {
    ValueType type;

    using value_t = std::variant<bool, double, Nil, String,
                                 Function, NativeFunction,
                                 Closure, Klass, Instance, BoundMethod, Module>;
    value_t as;

    template <class T>
    T get() {
        T val;

        try {
            val = std::get<T>(as);  // w contains int, not float: will throw
        } catch (const std::bad_variant_access &ex) {
            std::cout << "Execption :" << ex.what() << " : " << typeid(T).name() << '\n';
        }

        return val;
    }

    bool operator==(const Value &rhs) {
        return as == rhs.as;
    }
};

typedef Value (*NativeFn)(int argCount, Value *args);

struct ObjNative {
    NativeFn function;
    ObjNative(NativeFn native);
};

struct ObjUpvalue {
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
    ObjUpvalue(Value *slot);
};

struct ObjModule {
    std::vector<Value> variables;
    // Symbol table for the names of all module variables. Indexes here directly
    // correspond to entries in [variables].
    std::vector<String> variableNames;
    String name;
    ObjModule(String name);
};

struct ObjFunction {
    int arity;
    Chunk *chunk;
    std::string name;
    int upvalueCount;
    Module module;
    ObjFunction();
    ~ObjFunction();
};

enum FunctionType {
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_SCRIPT,
    TYPE_CONSTRUCTOR,
};

struct ObjClosure {
    Function function;
    ObjUpvalue **upvalues;
    int upvalueCount;
    ObjClosure(Function fn);
    ~ObjClosure();
};

using StringMap = std::unordered_map<String, Value>;
struct ObjClass {
    std::string name;
    StringMap methods;
    ObjClass(std::string name);
};

struct ObjInstance {
    Klass klass;
    StringMap fields;
    ObjInstance(Klass k);
};

struct ObjBoundMethod {
    Value receiver;
    Closure method;
    ObjBoundMethod(Value receiver, Closure method);
};

// custom specialization of std::hash can be injected in namespace std
struct ValueHash {
    std::size_t operator()(Value const &v) const noexcept {
        std::size_t h1 = std::hash<Value::value_t>{}(v.as);
        return h1;
    }
};

// using Entry = std::pair<std::string, Value>;

// using Table = std::unordered_map<Value, Value, ValueHash>;

#define BOOL_VAL(value) \
    Value { VAL_BOOL, value }
#define NIL_VAL     \
    Value {         \
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
#define BOUND_METHOD_VAL(value) \
    Value { VAL_BOUND_METHOD, value }
#define MODULE_VAL(value) \
    Value { VAL_MODULE, value }

#define IS_BOOL(value) (value.type == VAL_BOOL)
#define IS_NIL(value) (value.type == VAL_NIL)
#define IS_NUMBER(value) (value.type == VAL_NUMBER)
#define IS_STRING(value) (value.type == VAL_STRING)
#define IS_FUNCTION(value) (value.type == VAL_FUNCTION)
#define IS_NATIVE(value) (value.type == VAL_NATIVE)
#define IS_CLOSURE(value) (value.type == VAL_CLOSURE)
#define IS_CLASS(value) (value.type == VAL_CLASS)
#define IS_INSTANCE(value) (value.type == VAL_INSTANCE)
#define IS_BOUND_METHOD(value) (value.type == VAL_BOUND_METHOD)
#define IS_MODULE(value) (value.type == VAL_MODULE)

#define AS_BOOL(value) (value.get<bool>())
#define AS_NUMBER(value) (value.get<double>())
#define AS_STRING(value) (value.get<std::string>())
#define AS_CSTRING(value) (value.get<std::string>().c_str())
#define AS_FUNCTION(value) (value.get<Function>())
#define AS_NATIVEFN(value) (value.get<NativeFunction>()->function)
#define AS_CLOSURE(value) (value.get<Closure>())
#define AS_CLASS(value) (value.get<Klass>())
#define AS_INSTANCE(value) (value.get<Instance>())
#define AS_BOUND_METHOD(value) (value.get<BoundMethod>())
#define AS_MODULE(value) (value.get<Module>())

struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate n) const { std::cout << "nil"; }
    void operator()(const std::string &s) const { std::cout << s; }
    void operator()(const NativeFunction &n) const { std::cout << "<native fn>"; }
    void operator()(const Closure &c) const {
        operator()(c->function);
    }
    void operator()(const Function &f) const {
        if (f->name != "")
            std::cout << "<fn " << f->name << ">";
        else
            std::cout << "<srcipt>";
    }
    void operator()(const Klass &k) const { std::cout << k->name; }
    void operator()(const Instance &i) const { std::cout << i->klass->name << " instance"; }
    void operator()(const BoundMethod &b) const {
        operator()(b->method->function);
        ;
    }
    void operator()(const Module &n) const {
        operator()("module " + n->name);
        ;
    }
};

std::ostream &operator<<(std::ostream &os, const Value &v);
void printValue(Value value);

bool isFalsey(Value value);

bool valuesEqual(Value a, Value b);

std::string copyString(const char *chars, int length);