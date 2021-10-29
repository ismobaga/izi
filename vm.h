#pragma once


#include "chunk.h"
#include "value.h"
#include "compiler.h"

#define STACK_MAX 256

 enum InterpretResult{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} ;

 struct VM {
  Chunk* chunk;
  uint8_t* ip;
  std::vector<uint8_t>::iterator itip;
  Value stack[STACK_MAX];
  Value* stackTop;
  // cache string in memoire chap. 20
  // Table strings;

  Compiler compiler;

  VM();
  
  InterpretResult interpret(Chunk* chunk);
  InterpretResult interpret(const char* source);
  InterpretResult run();
  void resetStack();
  void runtimeError(const char* format, ...) ;
  void push(Value value);
  Value pop();
  Value peek(int distance) ;
} ;