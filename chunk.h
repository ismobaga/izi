#pragma once
#include "common.h"
#include "value.h"

#include <vector>


enum OpCode {
  CONSTANT,
  NIL,
  TRUE,
  FALSE,
  POP,
  EQUAL,
  GREATER,
  LESS,
  ADD,
  SUBTRACT,
  MULTIPLY,
  DIVIDE,
  NOT,
  NEGATE,
  PRINT,
  RETURN,

} ;

// smart line
// long constant
// https://github.com/munificent/craftinginterpreters/blob/master/note/answers/chapter14_chunks/
class Chunk{
  public: 
  std::vector<uint8_t> code;
  std::vector<int> lines;
  std::vector<Value> constants;
  public: 
  Chunk();
  size_t size(){return code.size();}
  void write(uint8_t  byte, int line);
  int addConstant(Value value);

  // debug function
  int disassembleInstruction(int offset);
  int constantInstruction(const char* name,                                int offset);
};