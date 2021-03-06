#include "compiler.h"

#include "stdio.h"

void Parser::errorAt(Token *token, const char *message) {
    if (panicMode)
        return;
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

void Parser::errorAtCurrent(const char *message) {
    errorAt(&current, message);
}
void Parser::error(const char *message) {
    errorAt(&previous, message);
}

void Parser::advance() {
    previous = current;

    for (;;) {
        current = scanner->scanToken();
        if (current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(current.start);
    }
}

void Parser::consume(TokenType type, const char *message) {
    if (current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}
bool Parser::check(TokenType type) {
    return current.type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type))
        return false;
    advance();
    return true;
}

Compiler::Compiler() {
    // initState(new CompilerState, TYPE_SCRIPT);
}
void Compiler::initState(CompilerState *compilerState, FunctionType type) {
    compilerState->enclosing = current;
    compilerState->localCount = 0;
    compilerState->scopeDepth = 0;
    compilerState->function = std::make_shared<ObjFunction>();
    compilerState->type = type;
    current = compilerState;
    if (type != TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start,
                                             parser.previous.length);
    }
    Local *local = &current->locals[current->localCount++];
    local->depth = 0;

    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
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

Function Compiler::compile(const char *source, Module module) {
    scanner.reset(source);
    CompilerState compiler;
    initState(&compiler, TYPE_SCRIPT);
    parser.scanner = &scanner;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    while (!parser.match(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_EOF, "Expect end of expression.");
    auto function = endCompiler();
    return parser.hadError ? NULL : function;
}

void Compiler::advance() { parser.advance(); }
void Compiler::consume(TokenType type, const char *message) { parser.consume(type, message); }
void Compiler::error(const char *message) {
    parser.error(message);
}

Chunk *Compiler::currentChunk() {
    return current->function->chunk;
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
    if (offset > UINT16_MAX)
        error("Loop body too large.");

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
    if (current->type == TYPE_CONSTRUCTOR) {
        emitBytes(OpCode::GET_LOCAL, 0);
    } else {
        emitByte(OpCode::NIL);
    }
    emitByte(OpCode::RETURN);
}
uint8_t Compiler::makeConstant(Value value) {
    int constant = currentChunk()->addConstant(value);
    if (constant > UINT8_MAX) {
        // TODO: LONG CONSTANT OU SHORT POUR CONSTANT
        error("Too many constants in one chunks");
        return 0;
    }

    return (uint8_t)constant;
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
void Compiler::emitConstant(Value value) {
    emitBytes(OpCode::CONSTANT, makeConstant(value));
}

Function Compiler::endCompiler() {
    emitReturn();
    Function function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != ""
                                             ? function->name.c_str()
                                             : "<script>");
    }
#endif
    current = current->enclosing;
    return function;
}
void Compiler::beginScope() {
    current->scopeDepth++;
}
void Compiler::endScope() {
    current->scopeDepth--;
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OpCode::CLOSE_UPVALUE);
        } else {
            emitByte(OpCode::POP);
        }
        current->localCount--;
    }
}
void Compiler::binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OpCode::EQUAL, OpCode::NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OpCode::EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OpCode::GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OpCode::LESS, OpCode::NOT);
            break;
        case TOKEN_LESS:
            emitByte(OpCode::LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OpCode::GREATER, OpCode::NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OpCode::ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OpCode::SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OpCode::MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OpCode::DIVIDE);
            break;
        default:
            return;  // Unreachable.
    }
}

void Compiler::call() {
    uint8_t argCount = argumentList();
    emitBytes(OpCode::CALL, argCount);
}

void Compiler::dot(bool canAssign) {
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OpCode::SET_PROPERTY, name);

    }
    /*
    // Add INVOKE : Optimize get method and call method
    else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
    */
    else {
        emitBytes(OpCode::GET_PROPERTY, name);
    }
}
void Compiler::literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(FALSE);
            break;
        case TOKEN_NIL:
            emitByte(NIL);
            break;
        case TOKEN_TRUE:
            emitByte(TRUE);
            break;
        default:
            return;  // Unreachable.
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
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OpCode::GET_UPVALUE;
        setOp = OpCode::SET_UPVALUE;
    }

    else {
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
Token Compiler::syntheticToken(const char *text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}
void Compiler::super_() {
    if (currentClass == NULL) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Can't use 'super' in a class with no superclass.");
    }
    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);

    namedVariable(syntheticToken("this"), false);
    namedVariable(syntheticToken("super"), false);
    emitBytes(OpCode::GET_SUPER, name);
}
void Compiler::this_() {
    if (currentClass == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }
    variable(false);
}
void Compiler::unary() {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    expression();

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OpCode::NEGATE);
            break;
        case TOKEN_BANG:
            emitByte(OpCode::NOT);
            break;
        default:
            return;  // Unreachable.
    }
}

void Compiler::expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}
void Compiler::block() {
    while (!parser.check(TOKEN_RIGHT_BRACE) && !parser.check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}
void Compiler::function(FunctionType type) {
    CompilerState cState;
    initState(&cState, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!parser.check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                parser.errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    Function func = endCompiler();
    emitBytes(OpCode::CLOSURE, makeConstant(FUNCTION_VAL(func)));
    for (int i = 0; i < func->upvalueCount; i++) {
        emitByte(cState.upvalues[i].isLocal ? 1 : 0);
        emitByte(cState.upvalues[i].index);
    }
}
void Compiler::method() {
    consume(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);
    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 3 &&
        memcmp(parser.previous.start, "new", 3) == 0) {
        type = TYPE_CONSTRUCTOR;
    }
    function(type);
    emitBytes(OpCode::METHOD, constant);
}

void Compiler::classDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OpCode::CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);
        if (className == parser.previous) {
            error("A class can't inherit from itself.");
        }
        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);
        namedVariable(className, false);
        emitByte(OpCode::INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!parser.check(TOKEN_RIGHT_BRACE) && !parser.check(TOKEN_EOF)) {
        method();
    }
    emitByte(OpCode::POP);
    if (classCompiler.hasSuperclass) {
        endScope();
    }
    currentClass = currentClass->enclosing;
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
}
void Compiler::funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}
void Compiler::varDeclaration() {
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

void Compiler::import() {
    consume(TOKEN_IDENTIFIER, "Expect a string after 'import'.");
    int moduleConstant = identifierConstant(&parser.previous);
    // Load
    emitBytes(OpCode::IMPORT, moduleConstant);

    // Discard the unused result value from calling the module body's closure.
    emitByte(OpCode::POP);
    // consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
}

void Compiler::expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OpCode::POP);
}
void Compiler::forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->size();
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OpCode::JUMP_IF_FALSE);
        emitByte(OpCode::POP);  // Condition.
    }
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OpCode::JUMP);
        int incrementStart = currentChunk()->size();
        expression();
        emitByte(OpCode::POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OpCode::POP);  // Condition.
    }
    endScope();
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

    if (match(TOKEN_ELSE))
        statement();

    patchJump(elseJump);
}

#define MAX_CASES 256

void Compiler::switchStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after value.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

    int state = 0;  // 0: before all cases, 1: before default, 2: after default.
    int caseEnds[MAX_CASES];
    int caseCount = 0;
    int previousCaseSkip = -1;

    while (!match(TOKEN_RIGHT_BRACE) && !parser.check(TOKEN_EOF)) {
        if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
            TokenType caseType = parser.previous.type;

            if (state == 2) {
                error("Can't have another case or default after the default case.");
            }

            if (state == 1) {
                // At the end of the previous case, jump over the others.
                caseEnds[caseCount++] = emitJump(OpCode::JUMP);

                // Patch its condition to jump to the next case (this one).
                patchJump(previousCaseSkip);
                emitByte(OpCode::POP);
            }

            if (caseType == TOKEN_CASE) {
                state = 1;

                // See if the case is equal to the value.
                emitByte(OpCode::DUP);
                expression();

                consume(TOKEN_COLON, "Expect ':' after case value.");

                emitByte(OpCode::EQUAL);
                previousCaseSkip = emitJump(OpCode::JUMP_IF_FALSE);

                // Pop the comparison result.
                emitByte(OpCode::POP);
            } else {
                state = 2;
                consume(TOKEN_COLON, "Expect ':' after default.");
                previousCaseSkip = -1;
            }
        } else {
            // Otherwise, it's a statement inside the current case.
            if (state == 0) {
                error("Can't have statements before any case.");
            }
            statement();
        }
    }

    // If we ended without a default case, patch its condition jump.
    if (state == 1) {
        patchJump(previousCaseSkip);
        emitByte(OpCode::POP);
    }

    // Patch all the case jumps to the end.
    for (int i = 0; i < caseCount; i++) {
        patchJump(caseEnds[i]);
    }

    emitByte(OpCode::POP);  // The switch value.
}

void Compiler::printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OpCode::PRINT);
}
void Compiler::returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_CONSTRUCTOR) {
            error("Can't return a value from an initializer.");
        }

        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OpCode::RETURN);
    }
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

void Compiler::synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;
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

            default:;  // Do nothing.
        }

        advance();
    }
}

void Compiler::declaration() {
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else if (match(TOKEN_IMPORT)) {
        import();
    } else {
        statement();
    }

    if (parser.panicMode)
        synchronize();
}

void Compiler::statement() {
    if (parser.match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_SWITCH)) {
        switchStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ParseRule *Compiler::getRule(TokenType type) {
    auto grouping = [this](bool canAssign) { this->grouping(); };
    auto unary = [this](bool canAssign) { this->unary(); };
    auto binary = [this](bool canAssign) { this->binary(); };
    auto call = [this](bool canAssign) { this->call(); };
    auto dot = [this](bool canAssign) { this->dot(canAssign); };
    auto number = [this](bool canAssign) { this->number(); };
    auto string = [this](bool canAssign) { this->string(); };
    auto literal = [this](bool canAssign) { this->literal(); };
    auto variable = [this](bool canAssign) { this->variable(canAssign); };
    auto super_ = [this](bool canAssign) { this->super_(); };
    auto this_ = [this](bool canAssign) { this->this_(); };
    auto and_ = [this](bool canAssign) { this->and_(); };
    auto or_ = [this](bool canAssign) { this->or_(); };
    static ParseRule rules[] = {
        [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, dot, PREC_CALL},
        [TOKEN_MINUS] = {unary, binary, PREC_TERM},
        [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG] = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
        [TOKEN_STRING] = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
        [TOKEN_AND] = {NULL, and_, PREC_AND},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, or_, PREC_OR},
        [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER] = {super_, NULL, PREC_NONE},
        [TOKEN_THIS] = {this_, NULL, PREC_NONE},
        [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
        [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    };

    return &rules[type];
}
void Compiler::parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression");
        return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}
// TODO : optimisser global value voir chapter21_global
uint8_t Compiler::identifierConstant(Token *name) {
    return makeConstant(STRING_VAL(copyString(name->start,
                                              name->length)));
    // TODO : Optimizasion ne marche pas
    // use <striing id> cache
}
int Compiler::resolveLocal(CompilerState *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (*name == local->name) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

int Compiler::addUpvalue(CompilerState *compiler, uint8_t index,
                         bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;
    for (int i = 0; i < upvalueCount; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }
    if (upvalueCount == UINT8_MAX) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

int Compiler::resolveUpvalue(CompilerState *compiler, Token *name) {
    if (current->enclosing == NULL)
        return -1;

    int local = resolveLocal(current->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}
void Compiler::declareVariable() {
    if (current->scopeDepth == 0)
        return;

    Token *name = &parser.previous;
    // peut pas avoir redefinition dans same scope
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
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
    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
    // local->depth = current->scopeDepth;
}
uint8_t Compiler::parseVariable(const char *errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous);
}

void Compiler::markInitialized() {
    if (current->scopeDepth == 0)
        return;
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

uint8_t Compiler::argumentList() {
    uint8_t argCount = 0;
    if (!parser.check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

void Compiler::and_() {
    int endJump = emitJump(OpCode::JUMP_IF_FALSE);

    emitByte(POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

void Compiler::or_() {
    int elseJump = emitJump(OpCode::JUMP_IF_FALSE);
    int endJump = emitJump(OpCode::JUMP);

    patchJump(elseJump);
    emitByte(OpCode::POP);
    // right operande
    parsePrecedence(PREC_OR);
    patchJump(endJump);
}