#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

static Token scanner_make_token(Scanner *scanner, TokenKind kind) {
  Token token;
  token.kind = kind;
  token.sub_kind = kind;
  token.start = scanner->start;
  token.length = (int)(scanner->current - scanner->start);
  token.line = scanner->line;

  return token;
}

static Token scanner_make_error_token(Scanner *scanner, const char *message) {
  Token token;
  token.kind = TokenKindError;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner->line;

  return token;
}

static char scanner_next_char(Scanner *scanner) {
  scanner->current++;
  return scanner->current[-1];
}

static bool scanner_at_end(Scanner *scanner) { return *scanner->current == '\0'; }

static char scanner_peek(Scanner *scanner) { return *scanner->current; }

static char scanner_peek_next(Scanner *scanner) {
  if (scanner_at_end(scanner))
    return '\0';
  return scanner->current[1];
}

static Token scanner_read_string(Scanner *scanner) {
  // Advance over the characters inside of the string
  while (scanner_peek(scanner) != '"' && !scanner_at_end(scanner)) {
    if (scanner_peek(scanner) == '\n')
      scanner->line++;

    if (scanner_peek(scanner) == '\\' && scanner_peek_next(scanner) == '\"') {
      // Include the escaped quote
      scanner_next_char(scanner);
    }

    scanner_next_char(scanner);
  }

  if (scanner_at_end(scanner))
    return scanner_make_error_token(scanner, "Unterminated string literal.");

  // Eat the final quote and return a string token
  scanner_next_char(scanner);
  return scanner_make_token(scanner, TokenKindString);
}

static bool scanner_is_digit(char c) { return c >= '0' && c <= '9'; }

static bool scanner_is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

static Token scanner_read_number(Scanner *scanner) {
  // Consume all digits until we hit something that isn't one
  while (scanner_is_digit(scanner_peek(scanner))) {
    scanner_next_char(scanner);
  }

  // Is there a decimal?
  if (scanner_peek(scanner) == '.' && scanner_is_digit(scanner_peek_next(scanner))) {
    // Eat the decimal and any remaining digits
    scanner_next_char(scanner);
    while (scanner_is_digit(scanner_peek(scanner))) {
      scanner_next_char(scanner);
    }
  }

  return scanner_make_token(scanner, TokenKindNumber);
}

static TokenKind scanner_check_keyword(Scanner *scanner, int start, int length, const char *rest,
                                       TokenKind kind) {
  if (scanner->current - scanner->start == start + length &&
      (length == 0 || memcmp(scanner->start + start, rest, length) == 0)) {
    return kind;
  }

  return TokenKindSymbol;
}

static TokenKind scanner_identifier_type(Scanner *scanner) {
  // Check for other specific keywords
  switch (scanner->start[0]) {
  case ':':
    return TokenKindKeyword;
  case 't':
    return scanner_check_keyword(scanner, 1, 0, "", TokenKindTrue);
  case 'n': {
    switch (scanner->start[1]) {
    case 'o':
      return scanner_check_keyword(scanner, 1, 2, "ot", TokenKindNot);
    case 'i':
      return scanner_check_keyword(scanner, 1, 2, "il", TokenKindNil);
    }
    break;
  }
  case 'e': {
    switch (scanner->start[1]) {
    case 'q': {
      switch (scanner->start[2]) {
      case 'v':
        return scanner_check_keyword(scanner, 3, 1, "?", TokenKindEqv);
      case 'u':
        return scanner_check_keyword(scanner, 3, 3, "al?", TokenKindEqual);
      }
    }
    }
    break;
  }
  case 'a':
    return scanner_check_keyword(scanner, 1, 2, "nd", TokenKindAnd);
  case 'o':
    return scanner_check_keyword(scanner, 1, 1, "r", TokenKindOr);
  case 'd': {
    switch (scanner->start[1]) {
    case 'e': {
      switch (scanner->start[2]) {
      case 'f': {
        switch (scanner->start[3]) {
        case 'i': {
          switch (scanner->start[4]) {
          case 'n': {
            switch (scanner->start[5]) {
            case 'e': {
              if (scanner->start[6] == '-') {
                switch (scanner->start[7]) {
                case 'm':
                  return scanner_check_keyword(scanner, 8, 5, "odule", TokenKindDefineModule);
                case 'r':
                  return scanner_check_keyword(scanner, 8, 10, "ecord-type",
                                               TokenKindDefineRecordType);
                }
              } else {
                return scanner_check_keyword(scanner, 6, 0, "", TokenKindDefine);
              }
            }
            }
          }
          }
        }
        }
      }
      }
      return scanner_check_keyword(scanner, 2, 4, "fine", TokenKindDefine);
    }
    case 'i':
      return scanner_check_keyword(scanner, 2, 5, "splay", TokenKindDisplay);
    }
    break;
  }
  case 's':
    return scanner_check_keyword(scanner, 1, 3, "et!", TokenKindSet);
  case 'l': {
    switch (scanner->start[1]) {
    case 'e':
      return scanner_check_keyword(scanner, 2, 1, "t", TokenKindLet);
    case 'a':
      return scanner_check_keyword(scanner, 2, 4, "mbda", TokenKindLambda);
    case 'i':
      return scanner_check_keyword(scanner, 2, 2, "st", TokenKindList);
    case 'o':
      return scanner_check_keyword(scanner, 2, 7, "ad-file", TokenKindLoadFile);
    }
  }
  case 'b':
    return scanner_check_keyword(scanner, 1, 4, "egin", TokenKindBegin);
  case 'i': {
    switch (scanner->start[1]) {
    case 'm':
      return scanner_check_keyword(scanner, 2, 4, "port", TokenKindImport);
    case 'f':
      return scanner_check_keyword(scanner, 2, 0, "", TokenKindIf);
    }
  }
  case 'c':
    return scanner_check_keyword(scanner, 1, 3, "ons", TokenKindCons);
  case 'm': {
    switch (scanner->start[1]) {
    case 'o': {
      switch (scanner->start[2]) {
      case 'd': {
        switch (scanner->start[3]) {
        case 'u': {
          switch (scanner->start[4]) {
          case 'l': {
            switch (scanner->start[5]) {
            case 'e': {
              switch (scanner->start[6]) {
              case '-': {
                switch (scanner->start[7]) {
                case 'i':
                  return scanner_check_keyword(scanner, 7, 6, "import", TokenKindModuleImport);
                case 'e':
                  return scanner_check_keyword(scanner, 7, 5, "enter", TokenKindModuleEnter);
                }
              }
              }
            }
            }
          }
          }
        }
        }
      }
      }
    }
    }
  }
  }

  return TokenKindSymbol;
}

static Token scanner_read_identifier(Scanner *scanner) {
  while (scanner_is_alpha(scanner_peek(scanner)) || scanner_is_digit(scanner_peek(scanner)) ||
         scanner_peek(scanner) == '-' || scanner_peek(scanner) == '?' ||
         scanner_peek(scanner) == '>' || scanner_peek(scanner) == '!') {
    scanner_next_char(scanner);
  }

  TokenKind kind = scanner_identifier_type(scanner);
  TokenKind sub_kind = TokenKindNone;
  if (kind != TokenKindSymbol && kind != TokenKindKeyword && kind != TokenKindTrue &&
      kind != TokenKindNil) {
    sub_kind = kind;
    kind = TokenKindSymbol;
  }

  Token token = scanner_make_token(scanner, kind);
  token.sub_kind = sub_kind;

  return token;
}

static void scanner_skip_whitespace(Scanner *scanner) {
  for (;;) {
    char c = scanner_peek(scanner);
    switch (c) {
    case ';':
      while (scanner_peek(scanner) != '\n' && !scanner_at_end(scanner))
        scanner_next_char(scanner);
      break;
    case '\n':
      scanner->line++;
      scanner_next_char(scanner);
      break;
    case ' ':
    case '\r':
    case '\t':
      scanner_next_char(scanner);
      break;
    default:
      return;
    }
  }
}

void mesche_scanner_init(Scanner *scanner, const char *source) {
  scanner->start = source;
  scanner->current = source;
  scanner->line = 1;
}

Token mesche_scanner_next_token(Scanner *scanner) {
  scanner_skip_whitespace(scanner);

  // Start the lexeme from the first real character
  scanner->start = scanner->current;

  if (scanner_at_end(scanner)) {
    return scanner_make_token(scanner, TokenKindEOF);
  }

  char c = scanner_next_char(scanner);

  if (scanner_is_alpha(c))
    return scanner_read_identifier(scanner);
  if (scanner_is_digit(c))
    return scanner_read_number(scanner);

  switch (c) {
  case '(':
    return scanner_make_token(scanner, TokenKindLeftParen);
  case ')':
    return scanner_make_token(scanner, TokenKindRightParen);
  case '"':
    return scanner_read_string(scanner);
  case '\'':
    return scanner_make_token(scanner, TokenKindQuote);
  case '`':
    return scanner_make_token(scanner, TokenKindBackquote);
  case ',':
    return scanner_make_token(scanner, TokenKindUnquote);
  case '@':
    return scanner_make_token(scanner, TokenKindSplice);
  case ':':
    return scanner_read_identifier(scanner);
  case '+':
    // TODO: Make sure this isn't the start of a symbol
    return scanner_make_token(scanner, TokenKindPlus);
  case '-': {
    if (scanner_is_digit(scanner_peek(scanner))) {
      return scanner_read_number(scanner);
    }

    return scanner_make_token(scanner, TokenKindMinus);
  }
  case '*':
    return scanner_make_token(scanner, TokenKindStar);
  case '/':
    return scanner_make_token(scanner, TokenKindSlash);
  case '%':
    return scanner_make_token(scanner, TokenKindPercent);
  case '>': {
    if (scanner_peek(scanner) == '=') {
      scanner_next_char(scanner);
      return scanner_make_token(scanner, TokenKindGreaterEqual);
    }

    return scanner_make_token(scanner, TokenKindGreaterThan);
  }
  case '<': {
    if (scanner_peek(scanner) == '=') {
      scanner_next_char(scanner);
      return scanner_make_token(scanner, TokenKindLessEqual);
    }

    return scanner_make_token(scanner, TokenKindLessThan);
  }
  }

  return scanner_make_error_token(scanner, "Unexpected character.");
}
