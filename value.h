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
struct ObjNativeClass;
struct ObjFunction;
struct ObjClosure;
struct ObjClass;
struct ObjInstance;
struct ObjNativeInstance;
struct ObjBoundMethod;
struct ObjModule;

using Nil = std::monostate;
using String = std::string;
using Function = std::shared_ptr<ObjFunction>;
using NativeFunction = std::shared_ptr<ObjNative>;
using Closure = std::shared_ptr<ObjClosure>;
using Klass = std::shared_ptr<ObjClass>;
using NativeClass = std::shared_ptr<ObjNativeClass>;
using Instance = std::shared_ptr<ObjInstance>;
using NativeInstance = std::shared_ptr<ObjNativeInstance>;
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
    VAL_NATIVE_CLASS,
    VAL_INSTANCE,
    VAL_BOUND_METHOD,
    VAL_MODULE,
};

enum ClassType
{
    CLS_BOOLEAN,
    CLS_COND_VAR,
    CLS_DATETIME,
    CLS_DURATION,
    CLS_ENUM,
    CLS_ENUM_VALUE,
    CLS_ENV,
    CLS_EXCEPTION,
    CLS_FILE,
    CLS_ITERABLE,
    CLS_ITERATOR,
    CLS_LIST,
    CLS_HASH,
    CLS_IMAGE,
    CLS_MODULE,
    CLS_MUTEX,
    CLS_NIL,
    CLS_NUMBER,
    CLS_OBJECT,
    CLS_SET,
    CLS_SOCKET,
    CLS_STRING,
    CLS_THREAD,
    CLS_USER_DEF,
} ;

enum Operator
{
    OPERATOR_MULTIPLICATION,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_DIVISION,
    OPERATOR_MODULO,
    OPERATOR_GREATER_THAN,
    OPERATOR_LESS_THAN,
    OPERATOR_GREATER_EQUAL,
    OPERATOR_LESS_EQUAL,
    OPERATOR_EQUALS,
    OPERATOR_INDEX,
    OPERATOR_INDEX_ASSIGN,
    OPERATOR_BITWISE_OR,
    OPERATOR_BITWISE_AND,
    OPERATOR_BITWISE_XOR,
    OPERATOR_BITWISE_NEGATE,
    OPERATOR_BITSHIFT_LEFT,
    OPERATOR_BITSHIFT_RIGHT,
    NUM_OPERATORS,
    OPERATOR_UNKNOWN,
} ;

/**
 * @brief IZI Value type
 *
 */
struct Value {
    ValueType type;

    using value_t = std::variant<bool, double, Nil, String,
                                 Function, NativeFunction,
                                 Closure, Klass, NativeClass, Instance, NativeInstance, BoundMethod, Module>;
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
    uint16_t optionalArguments[MAX_ARGS];
    uint8_t optionalArgCount;
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
    ClassType classType;
    bool final;
    ObjClass(std::string name, bool final = false);
};

typedef void (*NativeConstructor)(void *data);
typedef void (*NativeDestructor)(void *data);
typedef Value (*NativeMethod)(Value receiver, int argCount, Value *args);

struct ObjNativeClass {
    Klass klass;
    NativeConstructor constructor;
    NativeDestructor destructor;
    size_t allocSize;
    ObjNativeClass(
        std::string name,
        NativeConstructor constructor,
        NativeDestructor destructor,
        ClassType classType,
        size_t allocSize,
        bool final);
};

struct ObjNativeMethod {
    NativeMethod function;
    uint8_t arity;
    bool isStatic;
    Value name;
};

struct ObjInstance {
    Klass klass;
    StringMap fields;
    ObjInstance(Klass k);
};

struct ObjNativeInstance {
    ObjInstance instance;
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

extern ObjNativeInstance nil_instance;
extern ObjNativeInstance *boolean_true;
extern ObjNativeInstance *boolean_false;

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
#define NATIVE_CLASS_VAL(value) \
    Value { VAL_NATIVE_CLASS, value }
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
    void operator()(const NativeClass &k) const { std::cout << k->klass->name; }
    void operator()(const Instance &i) const { std::cout << i->klass->name << " instance"; }
    void operator()(const NativeInstance &i) const { std::cout << i->instance.klass->name << " instance"; }
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