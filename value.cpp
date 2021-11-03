#include "value.h"

#include <iterator>
#include <string>

#include "chunk.h"

ObjNative::ObjNative(NativeFn native) {
    function = native;
}

ObjUpvalue::ObjUpvalue(Value *slot) {
    location = slot;
    next = nullptr;
    closed = NIL_VAL;
}

ObjFunction::ObjFunction() {
    arity = 0;
    name = "";
    upvalueCount = 0;
    chunk = new Chunk();
}
ObjFunction::~ObjFunction() {
    delete chunk;
}

ObjClosure::ObjClosure(Function fn) {
    function = fn;
    upvalues = new ObjUpvalue *[fn->upvalueCount];
    upvalueCount = function->upvalueCount;
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = nullptr;
    }
}
ObjClosure::~ObjClosure() {
    delete[] upvalues;
}

ObjClass::ObjClass(std::string name) {
    this->name = name;
}

ObjInstance::ObjInstance(Klass k) {
    klass = k;
}

ObjBoundMethod::ObjBoundMethod(Value receiver, Closure method) {
    this->receiver = receiver;
    this->method = method;
}

inline std::ostream &operator<<(std::ostream &os, const Value &v) {
    std::visit(OutputVisitor(), v.as);
    return os;
}
void printValue(Value value) {
    std::cout << value;
    return;
}
struct FalsinessVisitor {
    bool operator()(const bool b) const { return !b; }
    bool operator()(const std::monostate n) const { return true; }

    template <typename T>
    bool operator()(const T &value) const { return false; }
};

bool isFalsey(Value value) {
    return std::visit(FalsinessVisitor(), value.as);
    // return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool valuesEqual(Value a, Value b) {
    return a.as == b.as;

    if (a.type != b.type)
        return false;
    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        default:
            return false;  // Unreachable.
    }
}

std::string copyString(const char *chars, int length) {
    return std::string(chars, length);
}