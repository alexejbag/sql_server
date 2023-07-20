// #pragma once
#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>

using std::string;

namespace lexer {
    extern const char * reservedWords[20]; // зарезервированные слова

    enum lexType {
        LEX_ALL,
        LEX_AND,
        LEX_CREATE,
        LEX_DELETE,
        LEX_DROP,
        LEX_FROM,
        LEX_IN,
        LEX_INSERT,
        LEX_INTO,
        LEX_LIKE,
        LEX_LONG_TYPE,
        LEX_NOT,
        LEX_OR,
        LEX_SELECT,
        LEX_SET,
        LEX_TABLE,
        LEX_TEXT_TYPE,
        LEX_UPDATE,
        LEX_WHERE,
        LEX_COMMA,
        LEX_CLOSE_PARENTHESIS,
        LEX_DIV,
        LEX_EQ,
        LEX_END,
        LEX_GEQ,
        LEX_GT,
        LEX_ID,
        LEX_LEQ,
        LEX_LT,
        LEX_MINUS,
        LEX_MOD,
        LEX_MULT,
        LEX_NEQ,
        LEX_NUM,
        LEX_OPEN_PARENTHESIS,
        LEX_PLUS,
        LEX_STR
    };

    struct Lexem {
        enum lexType type;
        string text;
    }; // такой структуры вектор command, содержащий введённую команду

    extern Lexem curLex; // текущая лексема
    extern int curPos;
    extern char c;
    
    int look(const string &str); // позиция в списке лексем (reservedWords)
    void getLex(const string &command); // получение очередной лексемы
}

#endif