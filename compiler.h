#pragma once

#include "scanner.h"
#include "chunk.h"
#include <functional>


#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

struct Parser {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
  Scanner *scanner;
  

  public:
  void errorAt(Token* token, const char* message) ;
  void errorAtCurrent(const char* message);
  void error(const char* message);
  void advance();
  void consume(TokenType type, const char * message);
  bool check(TokenType type) ;
  bool match(TokenType type) ;
} ;

 enum Precedence {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_CONDITIONAL, // ?:
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} ;

// typedef void (*ParseFn)();
typedef std::function<void(bool)> ParseFn;

struct ParseRule{
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
};
struct Local{
  Token name;
  int depth;
} ;

 struct CompilerState {
  Local locals[UINT8_MAX];
  int localCount;
  int scopeDepth;
} ;



class  Compiler{
Scanner scanner;
Chunk* compilingChunk;
Parser parser;
CompilerState* current = nullptr;
std::unordered_map<std::string, Value> stringConstants;



public: 
Compiler();
void initState(CompilerState *cs);
Chunk* currentChunk() ;

bool compile(const char* source, Chunk *chunk);
void advance();
void consume(TokenType type, const char * message);
bool match(TokenType type){ return parser.match(type);}
void error(const char* message) ;
void emitByte(uint8_t byte);
void emitBytes(uint8_t byte1, uint8_t byte2);
void emitLoop(int loopStart);
int emitJump(uint8_t instruction);
void emitReturn();
uint8_t makeConstant(Value value);
void patchJump(int offset);
void emitConstant(Value value);
void endCompiler();
void beginScope();
void endScope();
void binary();
void literal();
void grouping();
void number();
void string();
void namedVariable(Token name, bool canAssign);
void variable(bool canAssign);
void unary();
void expression();
void block();
void varDeclaration();
void expressionStatement();
void ifStatement();
void printStatement();
void whileStatement();
void synchronize(); /* Eviter a cascade d'erreur (apres statement)*/
void statement();
void declaration();
ParseRule* getRule(TokenType type) ;
void parsePrecedence(Precedence precedence);
uint8_t identifierConstant(Token* name) ;
void addLocal(Token name);
int resolveLocal(CompilerState* compilerState, Token* name) ;
void declareVariable();
uint8_t parseVariable(const char* errorMessage) ;
void markInitialized();
void defineVariable(uint8_t global) ;
void and_();
void or_();
};