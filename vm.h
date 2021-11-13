#pragma once

#include "chunk.h"
#include "compiler.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_MAX)

enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

using IPType = uint8_t *;  // std::vector<uint8_t>::iterator;

struct CallFrame {
    Closure closure;
    // IPType ip;
    int index;
    inline IPType getIp() {
        return closure->function->chunk->code.data() + index;
    }
    IPType inc(int i = 1) {
        index += i;
        return closure->function->chunk->code.data() + index - 1;
    }
    uint16_t readShort() {
        index += 2;

        return (uint16_t)((getIp()[-2] << 8) | getIp()[-1]);
    }
    Value *slots;
};

struct VM {
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    std::vector<uint8_t>::iterator itip;
    Value stack[STACK_MAX];
    Value *stackTop;

    std::unordered_map<std::string, Value> globals; /* hash table global variables*/
    // cache string in memoire chap. 20
    // Table strings;
    ObjUpvalue *openUpvalues;

    std::unordered_map<String, Value> modules;
    Module lastModule;

    Compiler compiler;

    String constructName;

    VM();

    InterpretResult interpret(Chunk *chunk);
    InterpretResult interpret(const char *source);
    InterpretResult run();
    void resetStack();
    void runtimeError(const char *format, ...);
    void defineNative(const char *name, NativeFn function);
    void push(Value value);
    Value pop();
    Value peek(int distance);
    bool call(Closure closure, int argCount);
    bool callValue(Value callee, int argCount);
    bool bindMethod(Klass klass, String name);
    ObjUpvalue *captureUpvalue(Value *local);
    void closeUpvalues(Value *last);
    void defineMethod(String name);
    Value importModule(Value name);
    Closure compileInModule(Value name, const char *source);
    Module getModule(Value name);
};

Value clockNative(int argCount, Value *args);

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}