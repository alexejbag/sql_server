// #pragma once
#ifndef DBMS_HPP
#define DBMS_HPP

#include <iostream>
#include <string>
#include <vector>

using std::string;
using std::vector;

#define MAX_FIELD_NAME_LEN 32

namespace dbms {
    enum FieldType {
        TEXT,
        LONG
    };

    enum Direction {
        LINK_PREV,
        LINK_NEXT
    };
    
    struct Field {
        char name[MAX_FIELD_NAME_LEN];
        FieldType type;
        long len;
    };

    struct FieldDef { // описание таблицы
        FieldType type;
        string name;
        unsigned long len;
    };

    struct TableDef {
        string name;
        long fieldsNum;                                
        vector<FieldDef> fieldsDef; // описание полей таблицы
    public:
        TableDef();
        void setName(const string & name);
        string getName();
        long getFieldsNum();
        FieldDef getField(long i);
        void addText(const string & fieldName, int fieldLength);
        void addLong(const string & fieldName);
    };

    struct TableInfo
    {
        size_t recordNumber;              
        long dataOffset; // data beginning offset
        long fieldNumber;               
        size_t recordSize;                
        size_t totalRecordNumber; // number of records including deleted records
        size_t firstRecordOffset;         
        size_t lastRecordOffset;          
        long firstDeletedOffset; // offset of first deleted record
    };

    struct Links
    {
        long prevOffset; // откуда начинается предыдущая запись
        long nextOffset; // откуда начинается последующая запись
    };

    class Table {
        string name;
        int fd;                                     
        Field* pFieldStruct; // массив полей
        struct TableInfo tableInfo; // информация о таблице
        struct Links links; // ссылки на предыдущие и последующие записи
        long currentPos; // current position in file
        void writeHeader();
        void readHeader();
        void modifyLinks(long position, long value, enum Direction dir);
    public:
        Table();
        ~Table();
        void createTable(TableDef & tableDef);                 
        void openTable(const string & tableName);              
        void closeTable();
        void insert(vector<char*> & newRecord); // запись в таблицу
        void getText(const string & fieldName, string & pvalue);
        void getLong(const string & fieldName, long & value);
        void putLong(const string & fieldName, long pvalue);
        void putText(const string & fieldName,const string & pvalue);
        void deleteRec();
        static void deleteTable(const string & tableName);
        void movePrevious();
        void moveNext();
        int getFieldLen(const string & fieldName);
        string getFieldNameByIndex(int i);
        long getRecNum();
        FieldType getFieldType(string & fieldName);
        int getFieldsNum();
    };
}

#endif