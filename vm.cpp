#include "vm.h"
#include "debug.h"
#include <stdarg.h>

VM::VM()
{
  resetStack();
}

InterpretResult VM::interpret(const char *source)
{
  Function function = compiler.compile(source);
  if (function == nullptr)
    return INTERPRET_COMPILE_ERROR;

  push(FUNCTION_VAL(function));
  CallFrame *frame = &frames[frameCount++];
  frame->function = function;
  frame->ip = function->chunk->code.data();
  frame->slots = stack;
  
  return run();
}


InterpretResult VM::run()
{
  CallFrame* frame = &frames[frameCount - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT() (frame->function->chunk->constants[READ_BYTE()])
#define READ_SHORT() \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
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
    frame->function->chunk->disassembleInstruction((int)(frame->ip - frame->function->chunk->code.data()));

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
    case DUP: push(peek(0)); break;
    case GET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case SET_LOCAL:
    {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
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
      frame->ip += offset;
      break;
    }
    case JUMP_IF_FALSE:
    {
      uint16_t offset = READ_SHORT();
      if (isFalsey(peek(0)))
        frame->ip += offset;
      break;
    }
    case LOOP:
    {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
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
  frameCount = 0;
}

void VM::runtimeError(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  CallFrame *frame = &frames[frameCount-1];
  size_t instruction = frame->ip - frame->function->chunk->code.data() - 1;
  int line = frame->function->chunk->lines[instruction];
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