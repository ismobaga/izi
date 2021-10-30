#include "vm.h"
#include "debug.h"
#include <stdarg.h>

VM::VM()
{
  resetStack();
}

InterpretResult VM::interpret(const char *source)
{
  chunk = new Chunk();
  if (!compiler.compile(source, chunk))
  {
    delete chunk;
    chunk = nullptr;
    return INTERPRET_COMPILE_ERROR;
  }
  ip = this->chunk->code.data();
  itip = this->chunk->code.begin();

  InterpretResult result = run();
  delete chunk;
  chunk = nullptr;
  return result;
}

InterpretResult VM::interpret(Chunk *chunk)
{
  this->chunk = chunk;
  ip = this->chunk->code.data();
  itip = this->chunk->code.begin();
  return run();
}

InterpretResult VM::run()
{

#define READ_BYTE() (*itip++)
#define READ_CONSTANT() (chunk->constants[READ_BYTE()])
#define READ_SHORT() \
  (itip += 2, (uint16_t)((itip[-2] << 8) | itip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                    \
  do                                                \
  {                                                 \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
    {                                               \
      runtimeError("Operands must be numbers.");    \
      return INTERPRET_RUNTIME_ERROR;               \
    }                                               \
    double b = AS_NUMBER(pop());                    \
    double a = AS_NUMBER(pop());                    \
    push(valueType(a op b));                        \
  } while (false)

  for (;;)
  {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = stack; slot < stackTop; slot++)
    {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    chunk->disassembleInstruction((int)(itip - chunk->code.begin()));

#endif
    uint8_t instruction;
    switch (instruction = READ_BYTE())
    {
    case CONSTANT:
    {
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
    case GET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      push(stack[slot]);
      break;
    }
    case SET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      stack[slot] = peek(0);
      break;
    }
    case GET_GLOBAL:
    {
      auto name = READ_STRING();
      Value value;
      if (globals.find(name) == globals.end())
      {
        runtimeError("Undefined variable '%s'.", name.c_str());
        return INTERPRET_RUNTIME_ERROR;
      }
      value = globals[name];
      push(value);
      break;
    }
    case DEFINE_GLOBAL:
    {
      auto name = READ_STRING();
      globals[name] = peek(0);
      pop();
      break;
    }
    case SET_GLOBAL:
    {
      auto name = READ_STRING();

      if (globals.find(name) == globals.end())
      {
        runtimeError("Undefined variable '%s'.", name.c_str());
        return INTERPRET_RUNTIME_ERROR;
      }
      globals[name] = peek(0);
      break;
    }
    case EQUAL:
    {
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
    case ADD:
    {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1)))
      {
        std::string b = AS_STRING(pop());
        std::string a = AS_STRING(pop());
        push(STRING_VAL(a + b));
      }
      else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1)))
      {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      }
      else
      {
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
      if (!IS_NUMBER(peek(0)))
      {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      break;
    case PRINT:
    {
      printValue(pop());
      printf("\n");
      break;
    }
    case JUMP:
    {
      uint16_t offset = READ_SHORT();
      itip += offset;
      break;
    }
    case JUMP_IF_FALSE:
    {
      uint16_t offset = READ_SHORT();
      if (isFalsey(peek(0)))
        itip += offset;
      break;
    }
    case LOOP:
    {
      uint16_t offset = READ_SHORT();
      itip -= offset;
      break;
    }
    case OpCode::RETURN:
    {
      return INTERPRET_OK;
    }
    }
  }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_SHORT
#undef BINARY_OP
}

void VM::resetStack()
{
  stackTop = stack;
}

void VM::runtimeError(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = ip - chunk->code.data() - 1;
  int line = chunk->lines[instruction];
  fprintf(stderr, "[line %d] in script\n", line);
  resetStack();
}

void VM::push(Value value)
{
  *stackTop = value;
  stackTop++;
}

Value VM::pop()
{
  stackTop--;
  return *stackTop;
}

Value VM::peek(int distance)
{
  return stackTop[-1 - distance];
}