// #pragma once
#ifndef EXECUTER_HPP
#define EXECUTER_HPP

#include <string>
#include <vector>
#include <map>
#include "lexer.hpp"
#include "dbms.hpp"

using std::string;
using std::vector;
using std::map;

namespace executer {
    struct FieldInfo {
        dbms::FieldType type; // тип поля: text или long
        string textVal;
        long longVal;
    };

    void sqlSelect(vector<lexer::Lexem> &command);
    void sqlInsert(vector<lexer::Lexem> &command);
    void sqlUpdate(vector<lexer::Lexem> &command);
    void sqlDelete(vector<lexer::Lexem> &command);
    void sqlCreate(vector<lexer::Lexem> &command);
    void sqlDrop(vector<lexer::Lexem> &command);
    bool suitsWhere(map<string, FieldInfo> &record, vector<lexer::Lexem>& clause); // checks whether the current record suits WHERE-clause conditions or not
    vector<string> execute(vector<lexer::Lexem> &command);
}

#endif