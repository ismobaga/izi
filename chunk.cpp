#include "chunk.h"

int Chunk::simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

Chunk::Chunk() {
    code.reserve(10);
    lines.reserve(10);
    constants.reserve(10);
}

void Chunk::write(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

int Chunk::addConstant(Value value) {
    constants.push_back(value);
    return constants.size() - 1;
}

int Chunk::disassembleInstruction(int offset) {
    printf("%04d ", offset);

    if (offset > 0 &&
        lines[offset] == lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", lines[offset]);
    }

    OpCode instruction = (OpCode)code[offset];
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
        case GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", offset);
        case SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", offset);
        case OpCode::GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", offset);
        case OpCode::DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", offset);
        case OpCode::SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", offset);
        case GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", offset);
        case SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", offset);
        case GET_PROPERTY:
            return constantInstruction("OP_GET_PROPERTY", offset);
        case SET_PROPERTY:
            return constantInstruction("OP_SET_PROPERTY", offset);
        case GET_SUPER:
            return constantInstruction("OP_GET_SUPER", offset);
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
        case JUMP:
            return jumpInstruction("OP_JUMP", 1, offset);
        case JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, offset);
        case LOOP:
            return jumpInstruction("OP_LOOP", -1, offset);
        case CALL:
            return byteInstruction("OP_CALL", offset);
        case CLOSURE: {
            offset++;
            uint8_t constant = code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            printValue(constants[constant]);
            printf("\n");
            Function function = AS_FUNCTION(constants[constant]);
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = code[offset++];
                int index = code[offset++];
                printf("%04d      |                     %s %d\n",
                       offset - 2, isLocal ? "local" : "upvalue", index);
            }

            return offset;
        }
        case CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        case OpCode::RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OpCode::CLASS:
            return constantInstruction("OP_CLASS", offset);
        case METHOD:
            return constantInstruction("OP_METHOD", offset);
        case INHERIT:
            return simpleInstruction("OP_INHERIT", offset);
        case OpCode::IMPORT:
            return simpleInstruction("OP_IMPORT", offset);
            break;
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

int Chunk::constantInstruction(const char *name,
                               int offset) {
    int8_t constant = code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(constants[constant]);
    printf("'\n");
    return offset + 2;
}
int Chunk::byteInstruction(const char *name, int offset) {
    uint8_t slot = code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

int Chunk::jumpInstruction(const char *name, int sign, int offset) {
    uint16_t jump = (uint16_t)(code[offset + 1] << 8);
    jump |= code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}