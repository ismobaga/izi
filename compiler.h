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
typedef std::function<void()> ParseFn;

struct ParseRule{
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
};



class  Compiler{
Scanner scanner;
Chunk* compilingChunk;
Parser parser;



public: 
Chunk* currentChunk() ;

bool compile(const char* source, Chunk *chunk);
void advance();
void consume(TokenType type, const char * message);
bool match(TokenType type){ return parser.match(type);}
void error(const char* message) ;
void emitByte(uint8_t byte);
void emitBytes(uint8_t byte1, uint8_t byte2);
void emitReturn();
uint8_t makeConstant(Value value);
void emitConstant(Value value);
void endCompiler();
void binary();
void literal();
void grouping();
void number();
void string();
void unary();
void expression();
void varDeclaration();
void expressionStatement();
void printStatement() ;
void synchronize(); /* Eviter a cascade d'erreur (apres statement)*/
void statement();
void declaration();
ParseRule* getRule(TokenType type) ;
void parsePrecedence(Precedence precedence);
uint8_t identifierConstant(Token* name) ;
uint8_t parseVariable(const char* errorMessage) ;
};