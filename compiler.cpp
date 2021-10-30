#include "compiler.h"
#include "stdio.h"


void Parser::errorAt(Token* token, const char* message) {
  if(panicMode)  return ;
  panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  hadError = true;
}

 void Parser::errorAtCurrent(const char* message) {
  errorAt(&current, message);
}
 void Parser::error(const char* message) {
  errorAt(&previous, message);
}

void Parser::advance(){
  previous = current;

  for(;;){
    current = scanner->scanToken();
    if(current.type != TOKEN_ERROR) break;

    errorAtCurrent(current.start);
  }
}

void Parser::consume(TokenType type, const char * message){
  if(current.type == type){
    advance();
    return;
  }

  errorAtCurrent(message);
}
 bool Parser::check(TokenType type) {
  return current.type == type;
}

bool Parser::match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}


Compiler::Compiler(){
  // initState(new CompilerState);
  
}
void Compiler::initState(CompilerState *compilerState){
  compilerState->localCount = 0;
  compilerState->scopeDepth = 0;
  current = compilerState;
}
/*
bool Compiler::compile(const char* source, Chunk *chunk) {
  scanner.reset(source);
  int line = -1;
    for (;;) {
    Token token = scanner.scanToken();
    if (token.line != line) {
      printf("%4d ", token.line);
      line = token.line;
    } else {
      printf("   | ");
    }
    printf("%2d '%.*s'\n", token.type, token.length, token.start); 

    if (token.type == TOKEN_EOF) break;
  }
}*/

bool Compiler::compile(const char* source, Chunk *chunk) {
  scanner.reset(source);
  CompilerState compiler;
  initState(&compiler);
  compilingChunk = chunk;
  parser.scanner = &scanner;
  parser.hadError = false;
  parser.panicMode = false;
  advance();
  while (!parser.match(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_EOF, "Expect end of expression.");
  endCompiler();
  return !parser.hadError;
}

void Compiler::advance(){parser.advance();}
void Compiler::consume(TokenType type, const char * message){parser.consume(type, message);}
 void Compiler::error(const char* message) {
  parser.error(message);
}

Chunk* Compiler::currentChunk() {
  return compilingChunk;
}


void Compiler::emitByte(uint8_t byte) {
  currentChunk()->write(byte, parser.previous.line);
}
void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

void Compiler::emitLoop(int loopStart) {
  emitByte(OpCode::LOOP);

  int offset = currentChunk()->size() - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

int Compiler::emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->size() - 2;
}
void Compiler::emitReturn() {
  emitByte(OpCode::RETURN);
}
uint8_t Compiler::makeConstant(Value value){
  int constant = currentChunk()->addConstant(value);
  if(constant > UINT8_MAX){
    // TODO: LONG CONSTANT OU SHORT POUR CONSTANT
    error("Too many constants in one chunks");
    return 0;
  }

  return (uint8_t) constant;
}
void Compiler::patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->size() - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}
void Compiler::emitConstant(Value value){
  emitBytes(OpCode::CONSTANT, makeConstant(value));
}

void Compiler::endCompiler() {
  emitReturn();
  #ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}
void Compiler::beginScope() {
  current->scopeDepth++;
}
void Compiler::endScope() {
  current->scopeDepth--;
   while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    emitByte(OpCode::POP);
    current->localCount--;
  }
}
void Compiler::binary() {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OpCode::EQUAL, OpCode::NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OpCode::EQUAL); break;
    case TOKEN_GREATER:       emitByte(OpCode::GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OpCode::LESS, OpCode::NOT); break;
    case TOKEN_LESS:          emitByte(OpCode::LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OpCode::GREATER, OpCode::NOT); break;
    case TOKEN_PLUS:          emitByte(OpCode::ADD); break;
    case TOKEN_MINUS:         emitByte(OpCode::SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OpCode::MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OpCode::DIVIDE); break;
    default: return; // Unreachable.
  }
}
void Compiler::literal() {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(FALSE); break;
    case TOKEN_NIL: emitByte(NIL); break;
    case TOKEN_TRUE: emitByte(TRUE); break;
    default: return; // Unreachable.
  }
}
void Compiler::grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}
void Compiler::number() {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

void Compiler::string() {
  
  emitConstant(STRING_VAL(copyString(parser.previous.start + 1,
                                  parser.previous.length - 2)));
}
void Compiler::namedVariable(Token name, bool canAssign) {

uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OpCode::GET_LOCAL;
    setOp = OpCode::SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOp = OpCode::GET_GLOBAL;
    setOp = OpCode::SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}
void Compiler::variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}
void Compiler::unary() {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  expression();

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_MINUS: emitByte(OpCode::NEGATE); break;
    case TOKEN_BANG: emitByte(OpCode::NOT); break;
    default: return; // Unreachable.
  }
}



void Compiler::expression(){
  parsePrecedence(PREC_ASSIGNMENT);

}
void Compiler::block() {
  while (!parser.check(TOKEN_RIGHT_BRACE) && !parser.check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
void Compiler::varDeclaration(){
  uint8_t global = parseVariable("Expect variable name.");


  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OpCode::NIL);
  }
  consume(TOKEN_SEMICOLON,
          "Expect ';' after variable declaration.");

  defineVariable(global);
}
void Compiler::expressionStatement(){
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
  emitByte(OpCode::POP);
}

void Compiler::ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

  int thenJump = emitJump(JUMP_IF_FALSE);
  emitByte(POP);
  statement();

  

  int elseJump = emitJump(JUMP);

  patchJump(thenJump);
  emitByte(POP);

  if (match(TOKEN_ELSE)) statement();

  patchJump(elseJump);
}

 void Compiler::printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OpCode::PRINT);
}
 void Compiler::whileStatement() {
   int loopStart = currentChunk()->size();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OpCode::JUMP_IF_FALSE);
  emitByte(OpCode::POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OpCode::POP);
}

void Compiler::synchronize(){
    parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}

void Compiler::declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();

}


void Compiler::statement(){
  if (parser.match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  }
   else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  }
  else{
    expressionStatement();
  }
}

ParseRule* Compiler::getRule(TokenType type) {
  auto grouping = [this](bool canAssign) { this->grouping(); };
  auto unary = [this](bool canAssign) { this->unary(); };
  auto binary = [this](bool canAssign) { this->binary( ); };
  auto number = [this](bool canAssign) { this->number(); };
  auto string = [this](bool canAssign) { this->string(); };
  auto literal = [this](bool canAssign) { this->literal(); };
  auto variable = [this](bool canAssign) { this->variable(canAssign); };
  auto and_ = [this](bool canAssign) { this->and_(); };
  auto or_ = [this](bool canAssign) { this->or_(); };
  static ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,   PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary,   PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,   PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

  return &rules[type];
}
void Compiler::parsePrecedence(Precedence precedence){
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if(prefixRule == nullptr){
    error("Expect expression");
    return;

  }
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);
  while(precedence <= getRule(parser.current.type)->precedence){
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }

  
}
// TODO : optimisser global value voir chapter21_global
uint8_t Compiler::identifierConstant(Token* name) {
  auto str = STRING_VAL(copyString(name->start,
                                         name->length));
  auto it = stringConstants.find(AS_STRING(str)) ;
  if(it != stringConstants.end()){
    return (uint8_t)AS_NUMBER(it->second);
  }
  uint8_t index = makeConstant(str);
  stringConstants[AS_STRING(str)] =  NUMBER_VAL((double)index);
  return index;
}
int Compiler::resolveLocal(CompilerState* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (*name== local->name) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}
 void Compiler::declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;
  // peut pas avoir redefinition dans same scope
    for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; 
    }

    if (*name == local->name) {
      error("Already a variable with this name in this scope.");
    }
  }
  addLocal(*name);
}
void Compiler::addLocal(Token name) {
   if (current->localCount == UINT8_MAX) {
    error("Too many local variables in function.");
    return;
  }
  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  // local->depth = current->scopeDepth;
}
uint8_t Compiler::parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

 void Compiler::markInitialized() {
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}

void Compiler::defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OpCode::DEFINE_GLOBAL, global);
}

void Compiler::and_() {
  int endJump = emitJump(OpCode::JUMP_IF_FALSE);

  emitByte(POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

void Compiler::or_(){
  int elseJump = emitJump(OpCode::JUMP_IF_FALSE);
  int endJump = emitJump(OpCode::JUMP);

  patchJump(elseJump);
  emitByte(OpCode::POP);
  // right operande
  parsePrecedence(PREC_OR);
  patchJump(endJump);
}