#pragma once
#include <vector>

#include "common.h"
#include "value.h"

/**
 * @brief 
 * 
 */
enum OpCode {
    CONSTANT,
    NIL,
    TRUE,
    FALSE,
    POP,
    DUP,
    GET_LOCAL,
    SET_LOCAL,
    GET_GLOBAL,
    DEFINE_GLOBAL,
    SET_GLOBAL,
    GET_UPVALUE,
    SET_UPVALUE,
    GET_PROPERTY,
    SET_PROPERTY,
    GET_SUPER,
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
    JUMP,
    JUMP_IF_FALSE,
    LOOP,
    CALL,
    CLOSURE,
    CLOSE_UPVALUE,
    RETURN,
    CLASS,
    METHOD,
    INHERIT,

};

// smart line
// long constant
// https://github.com/munificent/craftinginterpreters/blob/master/note/answers/chapter14_chunks/

/**
 * @brief Store code (bytecode) and data (value)
 *
 * 
 */
class Chunk {
   public:
    //! Bytecode of instructions
    std::vector<uint8_t> code;
    //! Line information of instruction
    std::vector<int> lines;
    //! The constant pool for lookup value 
    std::vector<Value> constants;

   public:
    Chunk();
    size_t size() { return code.size(); }
    /**
     * @brief Append a byte to the end of the chunk
     * 
     * @param byte bytecode to add
     * @param line line number
     */
    void write(uint8_t byte, int line);
    int addConstant(Value value);

    // debug function
    int simpleInstruction(const char *name, int offset);
    int disassembleInstruction(int offset);
    int constantInstruction(const char *name, int offset);
    int byteInstruction(const char *name, int offset);
    int jumpInstruction(const char *name, int sign, int offset);
};