#include "chunk.h"

static int simpleInstruction(const char* name, int offset) {
  printf("%s\n", name);
  return offset + 1;
}




Chunk:: Chunk(){
  code.reserve(10);
  lines.reserve(10);
}

 void Chunk::write(uint8_t  byte, int line){
   code.push_back(byte);
   lines.push_back(line);
 }

 int Chunk::addConstant( Value value) {
  constants.push_back(value);
  return constants.size() - 1;
}




 int Chunk::disassembleInstruction(int offset){
     printf("%04d ", offset);

     if (offset > 0 &&
      lines[offset] == lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", lines[offset]);
  }

  uint8_t instruction = code[offset];
  switch (instruction) {
    
    case OpCode::CONSTANT:
      return constantInstruction("OP_CONSTANT", offset);
    case OpCode::NIL:
      return simpleInstruction("OP_NIL", offset);
    case OpCode::TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OpCode::FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OpCode::POP:
      return simpleInstruction("OP_POP", offset);
    case EQUAL:
      return simpleInstruction("OP_EQUAL", offset);
    case GREATER:
      return simpleInstruction("OP_GREATER", offset);
    case LESS:
      return simpleInstruction("OP_LESS", offset);
    case OpCode::ADD:
      return simpleInstruction("OP_ADD", offset);
    case SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case NOT:
      return simpleInstruction("OP_NOT", offset);
    case NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case PRINT:
      return simpleInstruction("OP_PRINT", offset);
    case OpCode::RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      printf("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
 }

 int Chunk::constantInstruction(const char* name,
                               int offset){
                                 int8_t constant = code[offset + 1];
  printf("%-16s %4d '", name, constant);
  printValue(constants[constant]);
  printf("'\n");
  return  offset + 2;
}