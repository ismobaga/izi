#pragma once

#include "chunk.h"
#include "value.h"
#include "compiler.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_MAX)

enum InterpretResult
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

using IPType = std::vector<uint8_t>::iterator;

struct CallFrame
{
  Closure closure;
  IPType ip;
  Value *slots;
};

struct VM
{
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  std::vector<uint8_t>::iterator itip;
  Value stack[STACK_MAX];
  Value *stackTop;

  std::unordered_map<std::string, Value> globals; /* hash table global variables*/
  // cache string in memoire chap. 20
  // Table strings;
  ObjUpvalue* openUpvalues;

  Compiler compiler;

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
  ObjUpvalue* captureUpvalue(Value* local) ;
  void closeUpvalues(Value* last) ;
};

Value clockNative(int argCount, Value *args);