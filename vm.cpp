#include "vm.h"

#include <stdarg.h>

#include "debug.h"

VM::VM() {
    constructName = "new";
    resetStack();
    defineNative("clock", clockNative);
}

InterpretResult VM::interpret(const char *source) {
    Function function = compiler.compile(source);
    if (function == nullptr)
        return INTERPRET_COMPILE_ERROR;

    push(FUNCTION_VAL(function));
    Closure closure = std::make_shared<ObjClosure>(function);
    pop();
    push(CLOSURE_VAL(closure));

    call(closure, 0);

    return run();
}

InterpretResult VM::run() {
    CallFrame *frame = &frames[frameCount - 1];

// #define READ_BYTE() *frame->ip++
#define READ_BYTE() *frame->inc()
#define READ_CONSTANT() \
    frame->closure->function->chunk->constants[READ_BYTE()]
// #define READ_SHORT() \
  // (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_SHORT() \
    (frame->index += 2, (uint16_t)((frame->getIp()[-2] << 8) | frame->getIp()[-1]))
#define READ_STRING() \
    AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                          \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value *slot = stack; slot < stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");

#endif
#ifdef DEBUG_PRINT_CODE
        // frame->closure->function->chunk->disassembleInstruction((int)(frame->ip - frame->closure->function->chunk->code.begin()));
        frame->closure->function->chunk->disassembleInstruction((int)(frame->getIp() - frame->closure->function->chunk->code.data()));
#endif  // DEBUG
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case NIL:
                push(NIL_VAL);
                break;
            case TRUE:
                push(BOOL_VAL(true));
                break;
            case FALSE:
                push(BOOL_VAL(false));
                break;
            case POP:
                pop();
                break;
            case DUP:
                push(peek(0));
                break;
            case GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case GET_GLOBAL: {
                std::string name = READ_STRING();
                Value value;
                auto it = globals.find(name);
                if (it == globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                value = it->second;
                push(value);
                break;
            }
            case DEFINE_GLOBAL: {
                auto name = READ_STRING();
                globals[name] = peek(0);
                pop();
                break;
            }
            case SET_GLOBAL: {
                auto name = READ_STRING();

                if (globals.find(name) == globals.end()) {
                    runtimeError("Undefined variable '%s'.", name.c_str());
                    return INTERPRET_RUNTIME_ERROR;
                }
                globals[name] = peek(0);
                break;
            }
            case GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Instance instance = AS_INSTANCE(peek(0));
                String name = READ_STRING();

                auto it = instance->fields.find(name);
                if (it != instance->fields.end()) {
                    pop();  // Instance.
                    push(it->second);
                    break;
                }
                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have fields.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Instance instance = AS_INSTANCE(peek(1));
                instance->fields[READ_STRING()] = peek(0);
                Value value = pop();
                pop();
                push(value);
                break;
            }
            case GET_SUPER: {
                String name = READ_STRING();
                Klass superclass = AS_CLASS(pop());

                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case GREATER:
                BINARY_OP(BOOL_VAL, >);
                break;
            case LESS:
                BINARY_OP(BOOL_VAL, <);
                break;
            case ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    std::string b = AS_STRING(pop());
                    std::string a = AS_STRING(pop());
                    push(STRING_VAL(a + b));
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                        "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case SUBTRACT:
                BINARY_OP(NUMBER_VAL, -);
                break;
            case MULTIPLY:
                BINARY_OP(NUMBER_VAL, *);
                break;
            case DIVIDE:
                BINARY_OP(NUMBER_VAL, /);
                break;
            case NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case NEGATE:
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            case JUMP: {
                uint16_t offset = READ_SHORT();
                // frame->ip += offset;
                frame->inc(offset);
                break;
            }
            case JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0)))
                    // frame->ip += offset;
                    frame->inc(offset);
                break;
            }
            case LOOP: {
                uint16_t offset = READ_SHORT();
                // frame->ip -= offset;
                frame->inc(-offset);
                break;
            }
            case CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &frames[frameCount - 1];
                break;
            }

            case CLOSURE: {
                Function function = AS_FUNCTION(READ_CONSTANT());
                Closure closure = std::make_shared<ObjClosure>(function);
                push(CLOSURE_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] =
                            captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case CLOSE_UPVALUE:
                closeUpvalues(stackTop - 1);
                pop();
                break;

            case OpCode::RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                frameCount--;
                if (frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                stackTop = frame->slots;
                push(result);
                frame = &frames[frameCount - 1];
                break;
            }
            case CLASS:
                push(CLASS_VAL(std::make_shared<ObjClass>(READ_STRING())));
                break;
            case METHOD:
                defineMethod(READ_STRING());
                break;
            case INHERIT: {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                Klass subclass = AS_CLASS(peek(0));
                for (auto [key, value] : AS_CLASS(superclass)->methods) {
                    subclass->methods[key] = value;
                }

                pop();  // Subclass.
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef BINARY_OP
}

void VM::resetStack() {
    stackTop = stack;
    frameCount = 0;
    openUpvalues = nullptr;
}

void VM::runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &frames[i];
        Function function = frame->closure->function;
        // size_t instruction = frame->ip - function->chunk->code.begin() - 1;
        size_t instruction = frame->getIp() - function->chunk->code.data() - 1;
        fprintf(stderr, "[line %d] in ",
                function->chunk->lines[instruction]);
        if (function->name == "") {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name.c_str());
        }
    }

    resetStack();
}

void VM::defineNative(const char *name, NativeFn function) {
    push(STRING_VAL(copyString(name, (int)strlen(name))));
    NativeFunction nf = std::make_shared<ObjNative>(function);
    push(NATIVE_VAL(nf));
    globals[AS_STRING(stack[0])] = stack[1];
    pop();
    pop();
}

void VM::push(Value value) {
    *stackTop = value;
    stackTop++;
}

Value VM::pop() {
    stackTop--;
    return *stackTop;
}

Value VM::peek(int distance) {
    return stackTop[-1 - distance];
}

bool VM::call(Closure closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }
    if (frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &frames[frameCount++];
    frame->closure = closure;
    // frame->ip = closure->function->chunk->code.begin();
    frame->index = 0;
    frame->slots = stackTop - argCount - 1;
    return true;
}

bool VM::callValue(Value callee, int argCount) {
    switch (callee.type) {
        case VAL_BOUND_METHOD: {
            BoundMethod bound = AS_BOUND_METHOD(callee);
            stackTop[-argCount - 1] = bound->receiver;
            return call(bound->method, argCount);
        }
        case VAL_CLASS: {
            Klass klass = AS_CLASS(callee);
            stackTop[-argCount - 1] = INSTANCE_VAL(std::make_shared<ObjInstance>(klass));

            auto it = klass->methods.find(constructName);
            if (it != klass->methods.end()) {
                return call(AS_CLOSURE(it->second), argCount);
            } else if (argCount != 0) {  // Defaut constructor
                runtimeError("Expected 0 arguments but got %d.",
                             argCount);
                return false;
            }
            return true;
        }
        case VAL_CLOSURE:
            return call(AS_CLOSURE(callee), argCount);
        case VAL_NATIVE: {
            NativeFn native = AS_NATIVEFN(callee);
            Value result = native(argCount, stackTop - argCount);
            stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default:
            break;  // Non-callable object type.
    }
    runtimeError("Can only call functions and classes.");
    return false;
}
bool VM::bindMethod(Klass klass, String name) {
    auto it = klass->methods.find(name);
    if (it == klass->methods.end()) {
        runtimeError("Undefined property '%s'.", name.c_str());
        return false;
    }

    BoundMethod bound = std::make_shared<ObjBoundMethod>(peek(0),
                                                         AS_CLOSURE(it->second));
    pop();
    push(BOUND_METHOD_VAL(bound));
    return true;
}

ObjUpvalue *VM::captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = nullptr;
    ObjUpvalue *upvalue = openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = new ObjUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == nullptr) {
        openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    return createdUpvalue;
}

void VM::closeUpvalues(Value *last) {
    while (openUpvalues != nullptr &&
           openUpvalues->location >= last) {
        ObjUpvalue *upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}

void VM::defineMethod(String name) {
    Value method = peek(0);
    Klass klass = AS_CLASS(peek(1));
    klass->methods[name] = method;
    pop();
}

Value clockNative(int argCount, Value *args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}