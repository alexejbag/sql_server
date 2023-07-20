#include "lexer.hpp"
#include "executer.hpp"
#include "dbms.hpp"

#include <string>
#include <vector>
#include <regex>
#include <fstream>

using namespace executer;
using std::string;
using std::vector;
using std::stack;
using std::regex;
using std::map;

vector<string> executer::execute (vector<lexer::Lexem> & command) 
{
    vector<string> result;
    try { // опредение типа команды
        if (command[0].type == lexer::LEX_SELECT) {
            sqlSelect(command);
            result.push_back ("file");
            result.push_back ("select_res");
        } else if (command[0].type == lexer::LEX_INSERT) {
            sqlInsert(command);
            result.push_back ("OK");
        } else if (command[0].type == lexer::LEX_UPDATE) {
            sqlUpdate(command);
            result.push_back ("OK");
        } else if (command[0].type == lexer::LEX_DELETE) {
            sqlDelete(command);
            result.push_back ("OK");
        } else if (command[0].type == lexer::LEX_CREATE) {
            sqlCreate(command);
            result.push_back ("OK");
        } else {
            sqlDrop (command);
            result.push_back ("OK"); // command[0]
        }
    }
    catch (std::exception & except) {
        result.push_back ("ERROR!");
        result.push_back (except.what()); // // сообщение об ошибке
    }

    return result;
}

bool executer::suitsWhere (map<string, FieldInfo> & record, vector<lexer::Lexem> & clause)
{
    if (clause[0].text == "WHERE_ALL") {
        return true;
    } else {
        if (clause[0].text == "WHERE_LIKE") {
            map<string, FieldInfo>::const_iterator p = record.find(clause[1].text);
            
            if (p == record.end()) {
                throw std::logic_error("Incorrect command: wrong field name.");
            }
            if (p->second.type != dbms::TEXT) {
                throw std::logic_error("Incorrect command: expected text field name.");
            }     
            string pattern;
            string str = p->second.textVal;
            
            for (size_t i = 0; i < str.size(); ++i) {
                if (str[i] == '_') {
                    pattern += '.';
                } else if (str[i] == '%') {
                    pattern += ".*";
                } else if (str[i] == 92) {
                    pattern += 92;
                    pattern += 92;
                } else if (str[i] == '$') {
                    pattern += 92;
                    pattern += '$';
                } else if (str[i] == '.') {
                    pattern += 92;
                    pattern += '.';
                } else if (str[i] == '|') {
                    pattern += 92;
                    pattern += '|';
                } else if (str[i] == '?') {
                    pattern += 92;
                    pattern += '?';
                } else if(str[i] == '*') {
                    pattern += 92;
                    pattern += '*';
                } else if (str[i] == '+') {
                    pattern += 92;
                    pattern += '+';
                } else if (str[i] == '(') {
                    pattern += 92;
                    pattern += '(';
                } else if (str[i] == ')') {
                    pattern += 92;
                    pattern += ')';
                } else {
                    pattern += str[i];
                }
            }
            regex patternRegEx = regex(pattern);
            bool matches = regex_match(str, patternRegEx);
            if (clause[2].text == "NOT_LIKE") {
                return !matches;
            }
            return matches;
        } else if (clause[0].text == "WHERE_IN_TEXT") {
            string str;
            if (clause[1].type == lexer::LEX_STR) {
                str = clause[1].text;
            } else if (clause[1].type == lexer::LEX_ID) {
                map<string, FieldInfo>::const_iterator p = record.find(clause[1].text);
                
                if (p == record.end()) {
                    throw std::logic_error("Incorrect command: no such field name.");
                } 
                
                if (p->second.type != dbms::TEXT) {
                    throw std::logic_error("Incorrect command: no such field name of type text.");
                }
                
                str = p->second.textVal;
            }
            
            // searching for a string value in list    
            if (clause[2].text == "IN") {
                for (size_t i = 0; i < clause.size(); ++i) {
                    if (str == clause[i].text) {
                        return true;
                    }
                }
                return false;
            }
            
            // claus[2].text == "NOT_IN"
            bool inList = false;
            for (size_t i = 3; i < clause.size(); ++i) {
                if (str == clause[i].text) {
                    inList = true;
                }
            }
            return !inList;

        } else if (clause[0].text == "WHERE_IN_LONG") {
            int i = 1;
            stack<long> compStack;
            
            while (clause[i].type != lexer::LEX_IN) {
                if (clause[i].type == lexer::LEX_NUM) {
                    compStack.push(atoi(clause[i].text.c_str()));
                } else if (clause[i].type == lexer::LEX_ID) { 
                    map<string, FieldInfo>::const_iterator p = record.find(clause[i].text);
                    
                    if (p == record.end()) {
                        throw std::logic_error("Incorrect command: no such field name.");
                    }
                    if (p->second.type != dbms::LONG) {
                        throw std::logic_error("Incorrect command: no such field name of long type.");
                    }
                    compStack.push(p->second.longVal);
                } else if (clause[i].type == lexer::LEX_PLUS) {
                    long tmp = compStack.top();
                    compStack.pop();
                    compStack.top() += tmp;
                } else if (clause[i].type == lexer::LEX_MINUS) {
                    long tmp = compStack.top();
                    compStack.pop();
                    compStack.top() -= tmp;
                } else if (clause[i].type == lexer::LEX_MULT) {
                    long tmp = compStack.top();
                    compStack.pop();
                    compStack.top() *= tmp;
                } else if (clause[i].type == lexer::LEX_DIV) {
                    long tmp = compStack.top();
                    compStack.pop();
                    
                    if (tmp == 0) {
                        throw std::logic_error("Incorrect command: division by zero.");
                    }
                     
                    compStack.top() /= tmp;
                 } else {
                    //clause[i].type == lexer::LEX_MOD
                    long tmp = compStack.top();
                    compStack.pop();
                 
                    if (tmp == 0) {
                        throw std::logic_error("Incorrect command: division by zero.");
                    }
                    compStack.top() %= tmp;
                }
                i += 1;
            }
            long result = compStack.top();
            
            // searching for a long value in list
            i += 1;
            //got the first list element position
            if (clause[2].text == "IN") {
                for (size_t j = i; j < clause.size(); ++j) {
                    if (result == atoi(clause[j].text.c_str())) {
                        return true;
                    }
                }
                return false;
            }
        
            // clause[2].text == "NOT_IN"
            bool inList = false;
            for (size_t j = i; j < clause.size(); ++j) {
                if (result == atoi(clause[i].text.c_str())) {
                    inList = true;
                }
            }
            return !inList;
        } else {
            // where logic expression
            enum StackItemType {
                STACK_TEXT,
                STACK_LONG,
                STACK_BOOL
            };

            struct StackItem {
                StackItemType type; 
                string textVal;
                long longVal;
                bool boolVal;
            };

            stack<StackItem> compStack;
            size_t i = 1;
            while (i < clause.size()) {
                if (clause[i].type == lexer::LEX_NUM) {
                    StackItem tmpItem;
                    tmpItem.type = STACK_LONG;
                    tmpItem.longVal = atoi(clause[i].text.c_str());
                    compStack.push(tmpItem);
                } else if (clause[i].type == lexer::LEX_STR) {
                    StackItem tmpItem;
                    tmpItem.type = STACK_TEXT;
                    tmpItem.textVal = clause[i].text;
                    compStack.push(tmpItem);
                } else if (clause[i].type == lexer::LEX_ID) {
                    map<string, FieldInfo>::const_iterator p = record.find(clause[i].text);

                    if (p == record.end()) {
                        throw std::logic_error("Incorrect command: no such field name.");
                    }
                    
                    StackItem tmpItem;
                    if (p->second.type == dbms::TEXT) {
                        tmpItem.type = STACK_TEXT;
                        tmpItem.textVal = p->second.textVal;
                    } else {
                        // p->second.type == LONG
                        tmpItem.type = STACK_LONG;
                        tmpItem.longVal = p->second.longVal;
                    }

                    compStack.push(tmpItem);
                } else if (clause[i].type == lexer::LEX_PLUS) {
                    long tmp = compStack.top().longVal;
                    compStack.pop();
                    compStack.top().longVal += tmp;
                } else if (clause[i].type == lexer::LEX_MINUS) {
                    long tmp = compStack.top().longVal;
                    compStack.pop();
                    compStack.top().longVal -= tmp;
                } else if (clause[i].type == lexer::LEX_MULT) {
                    long tmp = compStack.top().longVal;
                    compStack.pop();
                    compStack.top().longVal *= tmp;
                } else if (clause[i].type == lexer::LEX_DIV) {
                    long tmp = compStack.top().longVal;
                    if (tmp == 0) {
                        throw std::logic_error("Incorrect command : division by zero.");
                    }
                    compStack.top().longVal /= tmp;
                } else if (clause[i].type == lexer::LEX_MOD) {
                    long tmp = compStack.top().longVal;
                    compStack.pop();
                    if (tmp == 0) {
                        throw std::logic_error("Incorrect command: division by zero.");
                    }
                    compStack.top().longVal %= tmp;
                } else if (clause[i].type == lexer::LEX_OR) {
                    bool tmp = compStack.top().boolVal;
                    compStack.pop();
                    compStack.top().boolVal |= tmp;
                } else if (clause[i].type == lexer::LEX_AND) {
                    bool tmp = compStack.top().boolVal;
                    compStack.pop();
                    compStack.top().boolVal &= tmp;
                } else if (clause[i].type == lexer::LEX_NOT) {
                    compStack.top().boolVal = !compStack.top().boolVal;
                } else if (clause[i].type == lexer::LEX_EQ ||
                           clause[i].type == lexer::LEX_GT ||
                           clause[i].type == lexer::LEX_LT ||
                           clause[i].type == lexer::LEX_GEQ ||
                           clause[i].type == lexer::LEX_LEQ ||
                           clause[i].type == lexer::LEX_NEQ) {
                           
                    if (compStack.top().type == STACK_TEXT) {
                        string tmp2 = compStack.top().textVal;
                        compStack.pop();
                        string tmp1 = compStack.top().textVal; 
                        
                        if (clause[i].type == lexer::LEX_EQ) {
                            compStack.top().boolVal = (tmp1 == tmp2);
                        } else if (clause[i].type == lexer::LEX_GT) {
                            compStack.top().boolVal = (tmp1 > tmp2);
                        } else if (clause[i].type == lexer::LEX_LT) {
                            compStack.top().boolVal = (tmp1 < tmp2);
                        } else if (clause[i].type == lexer::LEX_GEQ) {
                            compStack.top().boolVal = (tmp1 >= tmp2);
                        } else if (clause[i].type == lexer::LEX_LEQ) {
                            compStack.top().boolVal = (tmp1 <= tmp2);
                        } else {
                            //clause[i].type == lexer::LEX_NEQ
                            compStack.top().boolVal = (tmp1 != tmp2);
                        }    
                    } else {
                        //compStack.top().type == STACK_LONG
                        long tmp2 = compStack.top().longVal;
                        compStack.pop();
                        long tmp1 = compStack.top().longVal;
                        
                        if (clause[i].type == lexer::LEX_EQ) {
                            compStack.top().boolVal = (tmp1 == tmp2);
                        } else if (clause[i].type == lexer::LEX_GT) {
                            compStack.top().boolVal = (tmp1 > tmp2);
                        } else if (clause[i].type == lexer::LEX_LT) {
                            compStack.top().boolVal = (tmp1 < tmp2);
                        } else if (clause[i].type == lexer::LEX_GEQ) {
                            compStack.top().boolVal = (tmp1 >= tmp2);
                        } else if (clause[i].type == lexer::LEX_LEQ) {
                            compStack.top().boolVal = (tmp1 <= tmp2);
                        } else {
                            //clause[i].type == lexer::LEX_NEQ
                            compStack.top().boolVal = (tmp1 != tmp2);
                        }
                    }
                    
                    compStack.top().type = STACK_BOOL;                    
              }
              i += 1;
          }
          return compStack.top().boolVal;
       }
    }
}
          
void executer::sqlSelect (vector<lexer::Lexem> & command) { // ???????????????????
    int i = 0;
    while (command[i].type != lexer::LEX_FROM) {
        i += 1;
    }

    string tableName = command[i + 1].text; // поле text в структуре Lexem
    vector<lexer::Lexem> clause;
    clause.insert(clause.begin(), command.begin() + i + 2, command.end()); // позиция вставки; откуда начинаем и где заканчивает вставлять

    try {
        dbms::Table table;
        table.openTable(tableName);
        long recsNum = table.getRecNum(); // кол-во записей
        int fieldsNum = table.getFieldsNum(); // кол-во полей

        // writing fields name in temporary file
        std::ofstream tmpfile ("select_res"); // file has been created successfully
        if (!tmpfile) {
            // если временный файл не создался
            throw std::logic_error("Can't create a temporary file.");
        }
        
        if (command[1].type == lexer::LEX_MULT) { // значок звёздочка => чтение всех полей (полный список полей)
            for (int j = 0; j < fieldsNum; ++j) {
                string curFieldName = table.getFieldNameByIndex(j);
                size_t curFieldLen = table.getFieldLen(curFieldName);

                while (curFieldName.size() < curFieldLen) {
                    curFieldName += ' ';
                }
                tmpfile << curFieldName;
            }
        } else {
            for (int j = 1; j < i; ++j) {
                string curFieldName = command[j].text;
                size_t curFieldLen = table.getFieldLen(curFieldName); // возвращение длины поля

                while (curFieldName.size() < curFieldLen) {
                    curFieldName += ' '; // отступы в таблице
                }
                tmpfile << curFieldName;
            }
        }
        tmpfile << "\n";

        //preparing record to pass to suitsWhere()
        for (long j = 0; j < recsNum; ++j) { // цикл по кол-ву записей
            map<string, FieldInfo> curRec; // текущая запись

            for (int k = 0; k < fieldsNum; ++k) { // цил по кол-ву полей
                string curFieldName = table.getFieldNameByIndex(k);
                FieldInfo curFieldInfo;
                curFieldInfo.type = table.getFieldType(curFieldName);

                if (curFieldInfo.type == dbms::TEXT) {
                    table.getText(curFieldName, curFieldInfo.textVal);
                } else {
                    //LONG
                    table.getLong(curFieldName, curFieldInfo.longVal);
                }

                curRec[curFieldName] = curFieldInfo;
            }

            // checking whether current record suits WHERE-clause or not
            if (suitsWhere(curRec, clause)) { // для каждой записи возврат true or false
                if (command[1].type == lexer::LEX_MULT) {
                    for (int m = 0; m < fieldsNum; ++m) {
                        string curFieldName = table.getFieldNameByIndex(m);
                        
                        if (curRec[curFieldName].type == dbms::TEXT) {
                            tmpfile << curRec[curFieldName].textVal;

                            int fieldLen = table.getFieldLen(curFieldName);
                            for (int k = curRec[curFieldName].textVal.size(); k < fieldLen; ++k) {
                                tmpfile << ' ';
                            }
                        } else {
                            // LONG
                            tmpfile << curRec[curFieldName].longVal;
                            tmpfile << ' ';
                        }
                    }
                } else {
                    for (int m = 1; m < i; ++m) {
                        if (curRec[command[m].text].type == dbms::TEXT) {
                            tmpfile << curRec[command[m].text].textVal;

                            int fieldLen = table.getFieldLen(command[m].text);
                            for (int k = curRec[command[m].text].textVal.size(); k < fieldLen; ++k) {
                                tmpfile << ' ';
                            }
                        } else {
                            // LONG
                            tmpfile << curRec[command[m].text].longVal;
                            tmpfile << ' ';
                        }
                    }
                }
                tmpfile << "\n";
            }
            if (j != recsNum - 1) {
                table.moveNext(); // передвигаем "курсор", если находимся не на последней записи
            }
        }

        table.closeTable();
    }
    catch (std::exception &) {
        throw;    
    }
}
    
void executer::sqlInsert (vector<lexer::Lexem> & command) {
    try {
        dbms::Table table;
        table.openTable(command[1].text);
        int polesNum = command.size() - 2;
        if (polesNum != table.getFieldsNum()) {
            throw std::logic_error("Incorrect command: wrong fields nimber.");
        }

        vector<char *> newRecord;
        for (int i = 0; i < polesNum; ++i) {
            char *curPole = (char *)calloc(1, table.getFieldLen(table.getFieldNameByIndex(i))); // выделение памчти под длину текущего поля
            if (curPole == NULL) {
                throw std::logic_error("Can't allocate memory.");
            }

            if (command[i + 2].type == lexer::LEX_NUM) {
                long curLong = atol(command[i + 2].text.c_str()); // перевод в число
                memcpy(curPole, & curLong, sizeof(long)); // запись в бинарном виде (буфер символов)
            } else {
                //STR
                memcpy(curPole, command[i + 2].text.c_str(), command[i + 2].text.size());
            }

            newRecord.push_back(curPole); // формирование новой записи
        }

        table.insert(newRecord); // запись новой записи в таблицу
        for (int i = 0; i < polesNum; ++i) {
            free(newRecord[i]);
        }

        table.closeTable();

    }
    catch (std::exception &) {
        throw;
    }
}
    
void executer::sqlUpdate (vector<lexer::Lexem> & command) {
    string tableName = command[1].text;

    vector<lexer::Lexem> clause;
    int wherePos = 0;
    while (command[wherePos].type != lexer::LEX_WHERE) {
        ++wherePos;
    }
    clause.insert(clause.begin(), command.begin() + wherePos, command.end());

    try {
        dbms::Table table;
        table.openTable(tableName);
        long recsNum = table.getRecNum();
        int fieldsNum = table.getFieldsNum();
        dbms::FieldType fieldType = table.getFieldType(command[2].text);

        for (long j = 0; j < recsNum; ++j) {
            map<string, FieldInfo> curRec;
            
            //preparing record to pass to suitsWhere()
            for (int k = 0; k < fieldsNum; ++k) {
                string curFieldName = table.getFieldNameByIndex(k);
                FieldInfo curFieldInfo;
                curFieldInfo.type = table.getFieldType(curFieldName);

                if (curFieldInfo.type == dbms::TEXT) {
                    table.getText(curFieldName, curFieldInfo.textVal);
                } else {
                    //LONG
                    table.getLong(curFieldName, curFieldInfo.longVal);
                }

                curRec[curFieldName] = curFieldInfo;
            }

            // checking whether current record suits WHERE-clause or not
            if (suitsWhere(curRec, clause)) {
                if (fieldType == dbms::TEXT) {
                    string newValue;
                    if (command[3].type == lexer::LEX_STR) {
                        newValue = command[3].text;
                    } else {
                        //LEX_ID
                        table.getText(command[3].text, newValue);
                    }
                    
                    // New value writing
                    table.putText(command[2].text, newValue);
                    
                } else {
                    //LONG
                    
                    // RPN calculating
                    int i = 3;
                    stack<long> compStack;

                    while (command[i].type != lexer::LEX_WHERE) {
                        if (command[i].type == lexer::LEX_NUM) {
                            compStack.push(atoi(command[i].text.c_str()));
                        } else if (command[i].type == lexer::LEX_ID) { 
                            map<string, FieldInfo>::const_iterator p = curRec.find(command[i].text);
                
                            if (p == curRec.end()) {
                                throw std::logic_error("Incorrect command: no such field name.");
                            }   
                
                            if (p->second.type != dbms::LONG) {
                                throw std::logic_error("Incorrect command: no such field name of long type.");
                            }
                
                            compStack.push(p->second.longVal);
                        } else if (command[i].type == lexer::LEX_PLUS) {
                            long tmp = compStack.top();
                            compStack.pop();
                            compStack.top() += tmp;
                        } else if (command[i].type == lexer::LEX_MINUS) {
                            long tmp = compStack.top();
                            compStack.pop();
                            compStack.top() -= tmp;
                        } else if (command[i].type == lexer::LEX_MULT) {
                            long tmp = compStack.top();
                            compStack.pop();
                            compStack.top() *= tmp;
                        } else if (command[i].type == lexer::LEX_DIV) {
                            long tmp = compStack.top();
                            compStack.pop();
                
                            if (tmp == 0) {
                                throw std::logic_error("Incorrect command: division by zero.");
                            }
                 
                            compStack.top() /= tmp;
                        } else {
                            //clause[i].type == lexer::LEX_MOD
                            long tmp = compStack.top();
                            compStack.pop();
             
                            if (tmp == 0) {
                                throw std::logic_error("Incorrect command: division by zero.");
                            }
            
                            compStack.top() %= tmp;
                        }
                        ++i;
                    }
                   
                    // New value writing
                    table.putLong(command[2].text, compStack.top());   
                }
            }
            if (j != recsNum - 1) {
                table.moveNext();
            }
        }
        table.closeTable();
    }
    catch (std::exception &) {
        throw;    
    }
}
    
void executer::sqlDelete (vector<lexer::Lexem> & command) {
    string tableName = command[1].text;
    vector<lexer::Lexem> clause;
    clause.insert(clause.begin(), command.begin() + 2, command.end());

    try {
        dbms::Table table;
        table.openTable(tableName);
        long recsNum = table.getRecNum();
        int fieldsNum = table.getFieldsNum();

        long j = 0;
        while (j < recsNum) {
            map<string, FieldInfo> curRec;
            
            //preparing record to pass to suitsWhere()
            for (int k = 0; k < fieldsNum; ++k) {
                string curFieldName = table.getFieldNameByIndex(k);
                FieldInfo curFieldInfo;
                curFieldInfo.type = table.getFieldType(curFieldName);

                if (curFieldInfo.type == dbms::TEXT) {
                    table.getText(curFieldName, curFieldInfo.textVal);
                } else {
                    //LONG
                    table.getLong(curFieldName, curFieldInfo.longVal);
                }

                curRec[curFieldName] = curFieldInfo;
            }

            // checking whether current record suits WHERE-clause or not
            if (suitsWhere(curRec, clause)) {
                table.deleteRec();
                recsNum -= 1;
            }

            table.moveNext();
        }
        table.closeTable();
    }
    catch (std::exception &) {
        throw;    
    }
}

void executer::sqlCreate (vector<lexer::Lexem> & command) {
    try {
        dbms::TableDef tableDef;
        tableDef.setName(command[1].text); // "TABLE" не записываем в полиз

        size_t i = 2;
        while (i < command.size()) {
            if (command[i + 1].type == lexer::LEX_TEXT_TYPE) {
                tableDef.addText(command[i].text, atoi(command[i + 2].text.c_str()));
                i += 3;
            } else {
                tableDef.addLong(command[i].text);
                i += 2;
            }
        }

        dbms::Table table;
        table.createTable(tableDef);
    }
    catch (std::exception &) {
        throw;    
    }
}

void executer::sqlDrop (vector<lexer::Lexem> & command) {
    try {
        dbms::Table::deleteTable(command[1].text);
    }
    catch (std::exception &) {
        throw;
    }
}