#include "vm.h"
#include "debug.h"
#include <stdarg.h>

VM::VM()
{

  resetStack();
  defineNative("clock", clockNative);
}

InterpretResult VM::interpret(const char *source)
{
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

InterpretResult VM::run()
{
  CallFrame *frame = &frames[frameCount - 1];

#define READ_BYTE() *frame->ip++
#define READ_CONSTANT() \
  frame->closure->function->chunk->constants[READ_BYTE()]
#define READ_SHORT() \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_STRING() \
  AS_STRING(READ_CONSTANT())
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
#define DEBUG_TRACE_EXECUTION
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value *slot = stack; slot < stackTop; slot++)
    {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    frame->closure->function->chunk->disassembleInstruction((int)(frame->ip - frame->closure->function->chunk->code.begin()));

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
    case DUP:
      push(peek(0));
      break;
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
      std::string name = READ_STRING();
      Value value;
      auto it = globals.find(name);
      if (it == globals.end())
      {
        runtimeError("Undefined variable '%s'.", name.c_str());
        return INTERPRET_RUNTIME_ERROR;
      }
      value = it->second;
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
    case CALL:
    {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount))
      {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &frames[frameCount - 1];
      break;
    }

    case CLOSURE:
    {
      Function function = AS_FUNCTION(READ_CONSTANT());
      Closure closure = std::make_shared<ObjClosure>(function);
      push(CLOSURE_VAL(closure));
      break;
    }

    case OpCode::RETURN:
    {
      Value result = pop();
      frameCount--;
      if (frameCount == 0)
      {
        pop();
        return INTERPRET_OK;
      }

      stackTop = frame->slots;
      push(result);
      frame = &frames[frameCount - 1];
      break;
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

  for (int i = frameCount - 1; i >= 0; i--)
  {
    CallFrame *frame = &frames[i];
    Function function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk->code.begin() - 1;
    fprintf(stderr, "[line %d] in ",
            function->chunk->lines[instruction]);
    if (function->name == "")
    {
      fprintf(stderr, "script\n");
    }
    else
    {
      fprintf(stderr, "%s()\n", function->name.c_str());
    }
  }

  resetStack();
}

void VM::defineNative(const char *name, NativeFn function)
{
  push(STRING_VAL(copyString(name, (int)strlen(name))));
  NativeFunction nf = std::make_shared<ObjNative>(function);
  push(NATIVE_VAL(nf));
  globals[AS_STRING(stack[0])] = stack[1];
  pop();
  pop();
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

bool VM::call(Closure closure, int argCount)
{
  if (argCount != closure->function->arity)
  {
    runtimeError("Expected %d arguments but got %d.",
                 closure->function->arity, argCount);
    return false;
  }
  if (frameCount == FRAMES_MAX)
  {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame *frame = &frames[frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk->code.begin();
  frame->slots = stackTop - argCount - 1;
  return true;
}

bool VM::callValue(Value callee, int argCount)
{
  switch (callee.type)
  {
  case VAL_CLOSURE:
    return call(AS_CLOSURE(callee), argCount);
  case VAL_NATIVE:
  {
    NativeFn native = AS_NATIVEFN(callee);
    Value result = native(argCount, stackTop - argCount);
    stackTop -= argCount + 1;
    push(result);
    return true;
  }
  default:
    break; // Non-callable object type.
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

Value clockNative(int argCount, Value *args)
{
  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}