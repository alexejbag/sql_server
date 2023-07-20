// #pragma once
#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "dbms.hpp"

#include <string>
#include <vector>
#include <stack>

using std::string;
using std::vector;
using std::stack;

namespace parser {
    enum ExprType { // тип выражения
        TEXT_EXPR,
        LONG_EXPR,
        BOOL_EXPR
    };
    
    struct ExprInfo {
        ExprType type;
        bool isID;  
    };
    
    extern string cmd;
    extern vector<lexer::Lexem> poliz;
    extern stack<lexer::Lexem> polizStack;
    
    void sqlSentence(const string & command);
    void selectSentence();
    void insertSentence();
    void updateSentence();
    void deleteSentence();
    void createSentence();
    void dropSentence();
    void poleDefinition();
    void whereClause();
    ExprInfo expr();
    ExprInfo expr2();
    ExprInfo expr3();
    ExprType relationOrExpr();
    bool isLong();
    ExprType constList();
}

#endif