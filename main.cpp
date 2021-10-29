#include <iostream>
#include "vm.h"
#include "chunk.h"
#include "debug.h"

static void repl() {
   VM vm;
  char line[1024];
  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    vm.interpret(line);
  }
}
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
static void runFile(const char* path) {
   VM vm;
  char* source = readFile(path);
  InterpretResult result = vm.interpret(source);
  free(source); 

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {

 

  #ifdef false
  Chunk *chunk = new Chunk();

  int constant = chunk->addConstant( 1.2);
  chunk->write( CONSTANT, 123);
  chunk->write( constant, 123);

  constant =chunk-> addConstant( 3.4);
  chunk->write(CONSTANT, 123);
  chunk->write( constant, 123);

  chunk->write(OpCode::ADD, 123);

  constant = chunk->addConstant( 5.6);
  chunk->write(CONSTANT, 123);
  chunk->write( constant, 123);

  chunk->write( DIVIDE, 123);

  chunk->write( NEGATE, 123);
  
  chunk->write( RETURN, 123);

  vm.interpret(chunk);
  #endif

    if (argc == 1) {
    repl();
  } else if (argc == 2) {
    runFile(argv[1]);
  } else {
    fprintf(stderr, "Usage: izi [path]\n");
    exit(64);
  }

  // disassembleChunk(chunk, "test chunk");
  std::cout << "END!\n";
} 