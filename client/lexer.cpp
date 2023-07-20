#include "lexer.hpp"

#include <iostream>
#include <string>
#include <cstring>

using namespace lexer;
using std::string;

const char* lexer::reservedWords[20] = {
    "ALL",
    "AND",
    "CREATE",
    "DELETE",
    "DROP",
    "FROM",
    "IN",
    "INSERT",
    "INTO",
    "LIKE",
    "LONG",
    "NOT",
    "OR",
    "SELECT",
    "SET",
    "TABLE",
    "TEXT",
    "UPDATE",
    "WHERE",
    NULL
};

Lexem lexer::curLex;
int lexer::curPos;
char lexer::c;

// checks if the current lexem is a reserved word
int lexer::look(const string &str) {
    int i = 0;

    while (reservedWords[i]) {
        if (!strcmp(str.c_str(), reservedWords[i])) {
            return i;
        }
        ++i;
    }
    return -1;
}

void lexer::getLex(const string &command) {
    curLex.text.clear();

    enum stateType {
        INIT,
        NUM,
        STR,
        CLOSE_QUOTE,
        PLUS,
        MINUS,
        MULT,
        DIV,
        MOD,
        EQ,
        NEQ,
        CMP,
        COMMA,
        OPEN_PARENTHESIS,
        CLOSE_PARENTHESIS,
        ID,
        END,
        OK
    } state = INIT;

    while (state != OK) {
        switch (state) {
            case INIT:
                if (c == ' ') {

                } else if (isdigit(c)) {
                    state = NUM;
                } else if (c == '+') {
                    state = PLUS;
                } else if (c == '-') {
                    state = MINUS;
                } else if (c == '*') {
                    state = MULT;
                } else if (c == '/') {
                    state = DIV;
                } else if (c == '%') {
                    state = MOD;
                } else if (c == '=') {
                    state = EQ;
                } else if (c == '!') {
                    state = NEQ;
                } else if ( c == '>' || c == '<') {
                    state = CMP;
                } else if (c == ',') {
                    state = COMMA;
                } else if (c == '\'') {
                    state = STR;
                } else if (c == '(') {
                    state = OPEN_PARENTHESIS;
                } else if (c == ')') {
                    state = CLOSE_PARENTHESIS;
                } else if (isalpha(c)) {
                    state = ID;
                } else if (c == '\n') {
                    curLex.type = LEX_END;
                    state = OK;
                } else {
                    throw std::logic_error(
                        "Unexpected character with code " + std::to_string(c));
                }
                break;

            case NUM:
                if (std::isdigit(c)) {

                } else {
                    curLex.type = LEX_NUM;
                    state = OK;
                }
                break;
            
            case STR:
                if (c == '\'') {
                    state = CLOSE_QUOTE;
                }
                break;

            case CLOSE_QUOTE:
                curLex.type = LEX_STR;
                state = OK;
                break;

            case PLUS:
                curLex.type = LEX_PLUS;
                state = OK;
                break;

            case MINUS:
                curLex.type = LEX_MINUS;
                state = OK;
                break;

            case MULT:
                curLex.type = LEX_MULT;
                state = OK;
                break;

            case DIV:
                curLex.type = LEX_DIV;
                state = OK;
                break;

            case MOD:
                curLex.type = LEX_MOD;
                state = OK;
                break;

            case EQ:
                curLex.type = LEX_EQ;
                state = OK;
                break;

            case NEQ:
                if (c == '=') {
                    curLex.type = LEX_NEQ;
                    state = OK;
                } else {
                    throw c;
                }
                break;

            case CMP:
                if (c == '=') {
                    if (curLex.text == ">") {
                        curLex.type = LEX_GEQ;
                    } else {
                        curLex.type = LEX_LEQ;
                    }
                } else if (curLex.text == ">") {
                    curLex.type = LEX_GT;
                } else {
                    curLex.type = LEX_LT;
                }
                state = OK;
                break;

            case COMMA:
                curLex.type = LEX_COMMA;
                state = OK;
                break;

            case OPEN_PARENTHESIS:
                curLex.type = LEX_OPEN_PARENTHESIS;
                state = OK;
                break;

            case CLOSE_PARENTHESIS:
                curLex.type = LEX_CLOSE_PARENTHESIS;
                state = OK;
                break;

            case ID:
                int i;
                if (isalpha(c) || isdigit(c)) {

                } else if ((i = look(curLex.text)) >= 0) {
                    // найдено зарезервированное слово
                    curLex.type = lexType(i);
                    state = OK;
                } else {
                    curLex.type = LEX_ID; // "имя" таблицы
                    state = OK;
                }
                break;
            
            case OK:
            case END:
                break;
        }

        if (state != OK) {
            if (c != ' ') {
                curLex.text.push_back(c);
            }
                
            ++curPos;
            c = command[curPos];
        }
    }
}