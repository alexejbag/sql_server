#include "parser.hpp"
#include "lexer.hpp"
#include "dbms.hpp"

#include <string>
#include <vector>
#include <stack>

using namespace parser;
using std::string;
using std::vector;
using std::stack;

string parser::cmd;
vector<lexer::Lexem> parser::poliz;
stack<lexer::Lexem> parser::polizStack;

void parser::sqlSentence (const string & command) {
    cmd = command;
    try {
        if (lexer::curLex.type == lexer::LEX_SELECT) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
            selectSentence();
        } else if (lexer::curLex.type == lexer::LEX_INSERT) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
            insertSentence();
        } else if (lexer::curLex.type == lexer::LEX_UPDATE) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
            updateSentence();
        } else if (lexer::curLex.type == lexer::LEX_DELETE) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
            deleteSentence();
        } else if (lexer::curLex.type == lexer::LEX_CREATE) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
            createSentence();
        } else if (lexer::curLex.type == lexer::LEX_DROP) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
            dropSentence();
        } else  {
            throw std::logic_error("Unknown command.");
        }
    }
    catch (std::exception &) {
        throw;    
    }
    if (lexer::curLex.type != lexer::LEX_END) {
        throw std::logic_error("Excess text after the end of the command.");
    }
}


void parser::selectSentence()
{
    if (lexer::curLex.type == lexer::LEX_MULT) {
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);
    } else if (lexer::curLex.type == lexer::LEX_ID) {
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);

        while (lexer::curLex.type == lexer::LEX_COMMA) {
            lexer::getLex(cmd);

            if (lexer::curLex.type == lexer::LEX_ID) {
                poliz.push_back(lexer::curLex);
                lexer::getLex(cmd);
            } else {
                throw std::logic_error("Incorrect command: expected an identifier.");
            }
        }
    } else {
        throw std::logic_error("Incorrect command: expected a star sign or an identifiers list.");
    }

    if (lexer::curLex.type != lexer::LEX_FROM) {
        throw std::logic_error("Incorrect command: expected from after poles enumeration.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected an identifier.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    whereClause();
}

            
void parser::insertSentence()
{
    if (lexer::curLex.type != lexer::LEX_INTO) {
        throw std::logic_error("Incorrect command: expected lexem INTO.");
    }
    
    lexer::getLex(cmd);
    
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected an identifier.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    if (lexer::curLex.type == lexer::LEX_OPEN_PARENTHESIS) {
        lexer::getLex(cmd);
        
        if (lexer::curLex.type == lexer::LEX_STR || isLong()) {
            poliz.push_back(lexer::curLex);
            lexer::getLex(cmd);
        } else {
            throw std::logic_error("Incorrrect command: expected a long number or a string.");    
        }

        while (lexer::curLex.type == lexer::LEX_COMMA) {
            lexer::getLex(cmd);
            
            if (lexer::curLex.type == lexer::LEX_STR || isLong()) {
                poliz.push_back(lexer::curLex);
                lexer::getLex(cmd);
            } else {
                throw std::logic_error("Incorrect command: expected a long number or a string.");
            }
        }

        if (lexer::curLex.type != lexer::LEX_CLOSE_PARENTHESIS) {
            throw std::logic_error("Incorrect command: expected close parenthesis after poles values enumeration.");
        }
        lexer::getLex(cmd);
    } else {
        throw std::logic_error("Incorrect command: expected open parenthesis.");
    }
}

void parser::updateSentence()
{
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected an identificator.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    if (lexer::curLex.type != lexer::LEX_SET) {
        throw std::logic_error("Incorrect command: expected SET after table name."); 
    }

    lexer::getLex(cmd);
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected pole name.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    if (lexer::curLex.type != lexer::LEX_EQ) {
        throw std::logic_error("Incorrect command: expected equal sign.");
    }
    
    lexer::getLex(cmd);
    ExprInfo exprInfo = expr();
    if (exprInfo.type == BOOL_EXPR) {
        throw std::logic_error("Incorrect command: expected an expression of long or text type.");    
    }

    whereClause();
}

void parser::deleteSentence()
{
    if (lexer::curLex.type != lexer::LEX_FROM) {
        throw std::logic_error("Incorrect command: expected lexem FROM.");
    }

    lexer::getLex(cmd);
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected table name.");;
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    whereClause();
}


void parser::createSentence()
{
    if (lexer::curLex.type != lexer::LEX_TABLE) {
        throw std::logic_error("Incorrect command: expected lexem TABLE.");
    }
    
    lexer::getLex(cmd);
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected table name.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    if (lexer::curLex.type == lexer::LEX_OPEN_PARENTHESIS) {
        lexer::getLex(cmd);
        poleDefinition();

        while (lexer::curLex.type == lexer::LEX_COMMA) {
            lexer::getLex(cmd);
            poleDefinition();
        }

        if (lexer::curLex.type != lexer::LEX_CLOSE_PARENTHESIS) {
            throw std::logic_error("Incorrect command: expected close parenthesis.");
        }
        lexer::getLex(cmd);
    } else {
        throw std::logic_error("Incorrect command: expected open parenthesis.");
    }
}


void parser::dropSentence()
{
    if (lexer::curLex.type != lexer::LEX_TABLE) {
        throw std::logic_error("incorrect command: expected lexem TABLE.");
    }

    lexer::getLex(cmd);
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("incorrect command: expected table name.");
    }
    
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
}


void parser::poleDefinition()
{
    if (lexer::curLex.type != lexer::LEX_ID) {
        throw std::logic_error("Incorrect command: expected an identifier.");
    }
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);

    if (lexer::curLex.type == lexer::LEX_LONG_TYPE) {
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);
    } else if (lexer::curLex.type == lexer::LEX_TEXT_TYPE) {
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);
        
        if (lexer::curLex.type == lexer::LEX_OPEN_PARENTHESIS) {
            lexer::getLex(cmd);
            if (lexer::curLex.type == lexer::LEX_NUM) {
                poliz.push_back(lexer::curLex);
                
                lexer::getLex(cmd);
                if (lexer::curLex.type != lexer::LEX_CLOSE_PARENTHESIS) {
                    throw std::logic_error("Incorrecrt command: expected close parenthesis.");
                }
                lexer::getLex(cmd);
            } else {
                throw std::logic_error("Incorrect command: expected unsigned number.");
            }
        } else {
            throw std::logic_error("Incorrect command: expected open parenthesis.");
        }
    } else {
        throw std::logic_error("Incorrect command: expected pole definition.");
    }
}



void parser::whereClause()
{
    if (lexer::curLex.type != lexer::LEX_WHERE) {
        throw std::logic_error("incorrect command: expected lexem WHERE.");
    }
    poliz.push_back(lexer::curLex);
    int wherePos = poliz.size();
    lexer::getLex(cmd);

    if (lexer::curLex.type == lexer::LEX_ALL) {
        poliz[wherePos - 1].text += "_ALL";
        lexer::getLex(cmd);
    } else {
        ExprInfo exprInfo = expr();
        if (lexer::curLex.type != lexer::LEX_END) {
            // LIKE or IN
            bool withNot = false;
        
            if (lexer::curLex.type == lexer::LEX_NOT) {
                withNot = true;
                lexer::getLex(cmd);
            }
        
            if (lexer::curLex.type!= lexer::LEX_LIKE && lexer::curLex.type != lexer::LEX_IN) {
                throw std::logic_error("Incorrect command: expected LIKE or IN.");
            }
        
            if (withNot) {
                lexer::curLex.text = "NOT_" + lexer::curLex.text;
            }
        
            poliz.push_back(lexer::curLex);
        
            if (lexer::curLex.type == lexer::LEX_LIKE) {
                if (exprInfo.type != TEXT_EXPR || !exprInfo.isID) {
                    throw std::logic_error("Incorrect command: expected a text field identifier.");
                }
                
                poliz[wherePos - 1].text += "_LIKE";
                lexer::getLex(cmd);
            
                if (lexer::curLex.type !=lexer::LEX_STR) {
                    throw std::logic_error("INcorrect command: expected a string.");
                }
                
                lexer::getLex(cmd);
                
            } else {
                // IN
                if (exprInfo.type == BOOL_EXPR) {
                    throw std::logic_error("Incorrect command: expected an expresion of long or text type.");
                }
                
                poliz[wherePos - 1].text += "_IN";
                lexer::getLex(cmd);
                
                ExprType constListType;
                try {
                    constListType = constList();
                } catch (std::exception &) {
                    throw;
                }
            
                if (exprInfo.type != constListType) {
                    throw std::logic_error("Incorrect command: the expression and the list differ.");
                }

                if (exprInfo.type == TEXT_EXPR) {
                    poliz[wherePos - 1].text += "_TEXT"; // либо строка, либо длинное целое
                } else {
                    //LONG_EXPR
                    poliz[wherePos - 1].text += "_LONG"; // либо строка, либо длинное целое
                }
            }
        } else {
            // Boolean expression
            if (exprInfo.type != BOOL_EXPR) {
                throw std::logic_error("Incorrect command: expected a boolean expression.");
            }

            poliz[wherePos - 1].text += "_BOOL";
            lexer::getLex(cmd);   
        }
    }
}    

ExprInfo parser::expr()
{
    ExprInfo firstExpr2Info = expr2();
    ExprInfo secondExpr2Info;

    while (lexer::curLex.type == lexer::LEX_OR ||
           lexer::curLex.type == lexer::LEX_PLUS ||
           lexer::curLex.type == lexer::LEX_MINUS) {
           
        lexer::lexType op = lexer::curLex.type;
    
        if (firstExpr2Info.type == TEXT_EXPR) {
            throw std::logic_error("Incorrect command: expected long or std::logic expression.");
        }
    
        firstExpr2Info.isID = false;
        
        // RPN completing
        while (polizStack.size() && 
               polizStack.top().type != lexer::LEX_OPEN_PARENTHESIS &&
               polizStack.top().type != lexer::LEX_EQ &&
               polizStack.top().type != lexer::LEX_GT &&
               polizStack.top().type != lexer::LEX_LT &&
               polizStack.top().type != lexer::LEX_GEQ &&
               polizStack.top().type != lexer::LEX_LEQ &&
               polizStack.top().type != lexer::LEX_NEQ) {
               
            poliz.push_back(polizStack.top());
            polizStack.pop();
        }
        polizStack.push(lexer::curLex);
        
        // the second operand reading
        lexer::getLex(cmd);
        secondExpr2Info = expr2();
    
        if (firstExpr2Info.type == BOOL_EXPR && 
            (op != lexer::LEX_OR || secondExpr2Info.type == BOOL_EXPR)) {
            throw std::logic_error("Incorrect command: expected boolean expression.");
        }
    
        if (firstExpr2Info.type == LONG_EXPR && 
            (op == lexer::LEX_OR || secondExpr2Info.type != LONG_EXPR)) {
            throw std::logic_error("Incorrect command: expected expression of long type.");
        }
    
        firstExpr2Info = secondExpr2Info;
    }
    return firstExpr2Info;
}

ExprInfo parser::expr2()
{
    ExprInfo firstExpr3Info = expr3();
    ExprInfo secondExpr3Info;

    while (lexer::curLex.type == lexer::LEX_AND || 
           lexer::curLex.type == lexer::LEX_MULT || 
           lexer::curLex.type == lexer::LEX_DIV || 
           lexer::curLex.type == lexer::LEX_MOD) {
           
        lexer::lexType op = lexer::curLex.type;
        
        if (firstExpr3Info.type == TEXT_EXPR) {
            throw std::logic_error("Incorrect comand: expected an expression of long or boolean type.");
        }
       
        firstExpr3Info.isID = false;
        
        // RPN completing
        if (op == lexer::LEX_AND) {
            while (polizStack.size() && 
                      (polizStack.top().type == lexer::LEX_NOT ||
                           polizStack.top().type == lexer::LEX_AND)) {
                           
                poliz.push_back(polizStack.top());
                polizStack.pop();
            }
            polizStack.push(lexer::curLex);
        } else {
            // op is *, / or %
            polizStack.push(lexer::curLex);
        }
        
        //the second operand reading
        lexer::getLex(cmd);
        secondExpr3Info = expr3();
        
        if (firstExpr3Info.type == BOOL_EXPR && 
            (op != lexer::LEX_AND || secondExpr3Info.type != BOOL_EXPR)) {
            
            throw std::logic_error("Incorrect command: expected an expression of boolean type.");
            
        }
      
        if (firstExpr3Info.type == LONG_EXPR && 
            (op == lexer::LEX_AND || secondExpr3Info.type != LONG_EXPR)) {
            
            throw std::logic_error("Incorrect command: expected an expression of long type.");
        }
        firstExpr3Info = secondExpr3Info; 
    }
    return firstExpr3Info;
}

ExprInfo parser::expr3()
{
    ExprInfo expr3Info;
    if (lexer::curLex.type == lexer::LEX_STR) {
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);

        expr3Info.type = TEXT_EXPR;
        expr3Info.isID = false;
    } else if (isLong()) {
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);

        expr3Info.type = LONG_EXPR;
        expr3Info.isID = false; 
    } else if (lexer::curLex.type == lexer::LEX_ID) {
        poliz.push_back(lexer::curLex);

        expr3Info.isID = true;
        
        // defining identifier field type
        int tableNamePos;
        if (poliz[0].type == lexer::LEX_SELECT) {
            tableNamePos = 3;
            while (poliz[tableNamePos - 1].type != lexer::LEX_FROM) {
                ++tableNamePos;
            }
        } else {
            tableNamePos = 1;
        }

        try {
            dbms::Table table;
            table.openTable(poliz[tableNamePos].text);
            if (table.getFieldType(lexer::curLex.text) == dbms::TEXT) {
                expr3Info.type = TEXT_EXPR;
            } else {
                expr3Info.type = LONG_EXPR;
            }
        } catch (std::exception&) {
            throw;
        }
        
        lexer::getLex(cmd);
    } else if (lexer::curLex.type == lexer::LEX_NOT ||
                lexer::curLex.type == lexer::LEX_OPEN_PARENTHESIS) {
        bool withNot = false;
        
        if (lexer::curLex.type == lexer::LEX_NOT) {
            withNot = true;
            polizStack.push(lexer::curLex);
            lexer::getLex(cmd);
            
            while (lexer::curLex.type == lexer::LEX_NOT) {
                polizStack.push(lexer::curLex);
                lexer::getLex(cmd);
            }
        
            if (lexer::curLex.type != lexer::LEX_OPEN_PARENTHESIS) {
                throw std::logic_error("Incorrect commmand: expected an open parenthesis.");
            }
        }
        
        // open parenthesis to the polizStack pushing
        polizStack.push(lexer::curLex);

        lexer::getLex(cmd);
        expr3Info.type = relationOrExpr();
        
        if (expr3Info.type != BOOL_EXPR && withNot) {
            throw std::logic_error("Incorrect command: expected boolean expression after lexem NOT.");
        }
        
        expr3Info.isID = false;
        
        if (lexer::curLex.type != lexer::LEX_CLOSE_PARENTHESIS) {
            throw std::logic_error("Incorrect command: expected a close parenthesis.");
        }
        
        // RPN completing
        while (polizStack.top().type != lexer::LEX_OPEN_PARENTHESIS) {
            poliz.push_back(polizStack.top());
            polizStack.pop();
        }
        polizStack.pop();

        lexer::getLex(cmd);
        
    } else {
        throw std::logic_error("Incorrect command: expected a long number, or a string, or identifier, or expression in parenthesises.");
    }
    return expr3Info;    
} 

ExprType parser::relationOrExpr()
{
    ExprInfo firstExprInfo = expr();
    
    if (lexer::curLex.type != lexer::LEX_END) {
        if (lexer::curLex.type == lexer::LEX_EQ || 
            lexer::curLex.type == lexer::LEX_GT || 
            lexer::curLex.type == lexer::LEX_LT || 
            lexer::curLex.type == lexer::LEX_GEQ || 
            lexer::curLex.type == lexer::LEX_LEQ || 
            lexer::curLex.type == lexer::LEX_NEQ) {
            
            if (firstExprInfo.type == BOOL_EXPR) {
                throw std::logic_error("Incorrect command: expected an expression of long or text type after the relation sign.");
            }
            
            // RPN completing
            while (polizStack.size() && 
                   polizStack.top().type != lexer::LEX_OPEN_PARENTHESIS) {

                poliz.push_back(polizStack.top());
                polizStack.pop();
            }
            polizStack.push(lexer::curLex);
            
            // second relation operand reading
            lexer::getLex(cmd);
            ExprInfo secondExprInfo = expr();
            
            if (secondExprInfo.type != firstExprInfo.type) {
                throw std::logic_error("Incorrect command: the expressions types in relation are different.");
            }
            
            firstExprInfo.type = BOOL_EXPR;
        } else {
            throw std::logic_error("Incorrect command: expected a relation sign.");    
        }  
    }
    return firstExprInfo.type;    
}

bool parser::isLong() 
{
    if (lexer::curLex.type == lexer::LEX_NUM || 
            lexer::curLex.type == lexer::LEX_MINUS) {
        if (lexer::curLex.type == lexer::LEX_MINUS) {
            lexer::getLex(cmd);
        
            if (lexer::curLex.type != lexer::LEX_NUM) {
                return false;
            }
            
            lexer::curLex.text = "-" + lexer::curLex.text;
        }
        return true;
    }
    return false;
}

ExprType parser::constList()
{
    if (lexer::curLex.type != lexer::LEX_OPEN_PARENTHESIS) {
        throw std::logic_error("Incorect command: expected an open parenthesis.");
    }

    lexer::getLex(cmd);

    // defining list type
    lexer::lexType listType;

    if (lexer::curLex.type != lexer::LEX_STR && !isLong()) {
        throw std::logic_error("Incorrect command: expected a string or a number of long type.");
    }
    
    listType = lexer::curLex.type;
    poliz.push_back(lexer::curLex);
    lexer::getLex(cmd);
    
    while (lexer::curLex.type == lexer::LEX_COMMA) {
        lexer::getLex(cmd);
        if (lexer::curLex.type != lexer::LEX_STR && !isLong()) {
            throw std::logic_error("Incorrect command: expected a string or a number of long type.");
        }
    
        if (listType != lexer::curLex.type) {
            throw std::logic_error("Incorrec command: the types of list elements differ.");
        }
    
        poliz.push_back(lexer::curLex);
        lexer::getLex(cmd);
    }

    if (lexer::curLex.type != lexer::LEX_CLOSE_PARENTHESIS) {
        throw std::logic_error("Incorrect command: expected a close parenthesis.");
    }
    lexer::getLex(cmd);

    if (listType == lexer::LEX_STR) {
        return TEXT_EXPR;
    }

    return LONG_EXPR;
}