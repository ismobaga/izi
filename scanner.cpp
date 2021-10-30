#include "scanner.h"

bool isDigit(char c)
{
  return c >= '0' && c <= '9';
}

bool isAlpha(char c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         c == '_';
}

Scanner::Scanner()
{
  start = nullptr;
  current = nullptr;
  line = 1;
}
Scanner::Scanner(const char *source)
{
  start = source;
  current = source;
  line = 1;
}

void Scanner::reset(const char *source)
{
  start = source;
  current = source;
  line = 1;
}

Token Scanner::scanToken()
{
  skipWhitespace();
  start = current;

  if (isAtEnd())
    return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c))
    return identifier();
  if (isDigit(c))
    return number();

  switch (c)
  {
  case '(':
    return makeToken(TOKEN_LEFT_PAREN);
  case ')':
    return makeToken(TOKEN_RIGHT_PAREN);
  case '{':
    return makeToken(TOKEN_LEFT_BRACE);
  case '}':
    return makeToken(TOKEN_RIGHT_BRACE);
  case ';':
    return makeToken(TOKEN_SEMICOLON);
  case ',':
    return makeToken(TOKEN_COMMA);
  case '.':
    return makeToken(TOKEN_DOT);
  case '-':
    return makeToken(TOKEN_MINUS);
  case '+':
    return makeToken(TOKEN_PLUS);
  case '/':
    return makeToken(TOKEN_SLASH);
  case '*':
    return makeToken(match('*') ? TOKEN_STAR_STAR : TOKEN_STAR);
  case ':':
    return makeToken(TOKEN_COLON);
  case '?':
    return makeToken(TOKEN_QUESTION_MARK);
  case '!':
    return makeToken(
        match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return makeToken(
        match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return makeToken(
        match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return makeToken(
        match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return string();
  }

  return errorToken("Unexpected character.");
}

Token Scanner::makeToken(TokenType type)
{
  Token token;
  token.type = type;
  token.start = start;
  token.length = (int)(current - start);
  token.line = line;
  return token;
}

Token Scanner::errorToken(const char *message)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = line;
  return token;
}
void Scanner::skipWhitespace()
{
  for (;;)
  {
    char c = peek();
    switch (c)
    {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      line++;
      advance();
      break;
    case '/':
      if (peekNext() == '/')
      {
        // A comment goes until the end of the line.
        while (peek() != '\n' && !isAtEnd())
          advance();
      }
      else
      {
        return;
      }
      break;
    default:
      return;
    }
  }
}
TokenType Scanner::checkKeyword(int start, int length,
                                const char *rest, TokenType type)
{
  if (this->current - this->start == start + length &&
      memcmp(this->start + start, rest, length) == 0)
  {
    return type;
  }

  return TOKEN_IDENTIFIER;
}
TokenType Scanner::identifierType()
{
  switch (start[0])
  {
  case 'a':
    return checkKeyword(1, 2, "nd", TOKEN_AND);
  case 'c':
    if (current - start > 1)
    {
      switch (start[1])
      {
      case 'l':
        return checkKeyword(2, 3, "ass", TOKEN_CLASS);
      case 'a':
        return checkKeyword(2, 2, "se", TOKEN_CASE);
      }
    }
    break;
  case 'e':
    return checkKeyword(1, 3, "lse", TOKEN_ELSE);
  case 'd':
    return checkKeyword(1, 6, "efault", TOKEN_DEFAULT);
  case 'f':
    if (current - start > 1)
    {
      switch (start[1])
      {
      case 'a':
        return checkKeyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return checkKeyword(2, 1, "r", TOKEN_FOR);
      case 'u':
        return checkKeyword(2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  case 'i':
    return checkKeyword(1, 1, "f", TOKEN_IF);
  case 'n':
    return checkKeyword(1, 2, "il", TOKEN_NIL);
  case 'o':
    return checkKeyword(1, 1, "r", TOKEN_OR);
  case 'p':
    return checkKeyword(1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    if (current - start > 1)
    {
      switch (start[1])
      {
      case 'u':
        return checkKeyword(2, 3, "per", TOKEN_SUPER);
      case 'w':
        return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
      }
    }
    break;

  case 't':
    if (current - start > 1)
    {
      switch (start[1])
      {
      case 'h':
        return checkKeyword(2, 2, "is", TOKEN_THIS);
      case 'r':
        return checkKeyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  case 'v':
    return checkKeyword(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}
Token Scanner::identifier()
{
  while (isAlpha(peek()) || isDigit(peek()))
    advance();
  return makeToken(identifierType());
}

Token Scanner::number()
{
  while (isDigit(peek()))
    advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext()))
  {
    // Consume the ".".
    advance();

    while (isDigit(peek()))
      advance();
  }

  return makeToken(TOKEN_NUMBER);
}
Token Scanner::string()
{
  while (peek() != '"' && !isAtEnd())
  {
    if (peek() == '\n')
      line++;
    advance();
  }

  if (isAtEnd())
    return errorToken("Unterminated string.");

  // The closing quote.
  advance();
  return makeToken(TOKEN_STRING);
}

bool Scanner::isAtEnd()
{
  return *current == '\0';
}

char Scanner::advance()
{
  current++;
  return current[-1];
}
char Scanner::peek()
{
  return *current;
}

char Scanner::peekNext()
{
  if (isAtEnd())
    return '\0';
  return current[1];
}

bool Scanner::match(char expected)
{
  if (isAtEnd())
    return false;
  if (*current != expected)
    return false;
  current++;
  return true;
}
