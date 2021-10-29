#pragma once

#include <string>
#include <cstring>

enum TokenType{
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, 
  TOKEN_COLON, TOKEN_QUESTION_MARK,
  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  TOKEN_STAR, TOKEN_STAR_STAR,
  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_ERROR, TOKEN_EOF
} ;

struct Token {
  TokenType type;
  const char* start;
  int length;
  int line;
};

/***
*
* String inter  https://github.com/munificent/craftinginterpreters/blob/master/note/answers/chapter16_scanning.md
*/

struct Scanner {
  const char* start;
  const char* current;
  int line;
  Scanner();
  Scanner(const char *source);
  void reset(const char *source);
  Token scanToken();
  Token makeToken(TokenType type);
  Token errorToken(const char* message) ;
  bool isAtEnd();
  char advance();
  char peek();
  char peekNext();
  bool match(char expected);
  void skipWhitespace();
  
TokenType checkKeyword(int start, int length, const char* rest, TokenType type);
  TokenType identifierType();
  Token identifier() ;
  Token number();
  Token string();
};