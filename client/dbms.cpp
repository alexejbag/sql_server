#include "dbms.hpp"

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace dbms;
using std::string;

// TableDef class methods:
TableDef::TableDef() {
    fieldsNum = 0;
}

void TableDef::setName(const string & name) {
    this->name = name;
}

string TableDef::getName() {
    return name;
}

long TableDef::getFieldsNum() {
    return fieldsNum;
}

FieldDef TableDef::getField(long i) {
    return fieldsDef[i];
}

void TableDef::addText(const string & fieldName, int fieldLen) {
    FieldDef curField;
    curField.type = TEXT;
    curField.name = fieldName;
    curField.len = fieldLen;
    fieldsDef.push_back(curField);
    ++fieldsNum;
}

void TableDef::addLong(const string & fieldName) {
    FieldDef curField;
    curField.type = LONG;
    curField.name = fieldName;
    curField.len = sizeof(long);
    fieldsDef.push_back(curField);
    ++fieldsNum;
}

// Table class methods:
Table::Table() {
    fd = -1;
    currentPos = -1;
    tableInfo.recordNumber = 0;
    tableInfo.totalRecordNumber = 0;
    tableInfo.firstRecordOffset = -1;
    tableInfo.firstDeletedOffset = -1;
    tableInfo.lastRecordOffset = 0;
    tableInfo.fieldNumber = 0;
    tableInfo.recordSize = 0;
    tableInfo.dataOffset = 0;
    links.prevOffset = -1;
    links.nextOffset = 0;
    pFieldStruct = NULL;
}

Table::~Table() {
    if (pFieldStruct != NULL) {
        free(pFieldStruct);
    }
}

void Table::createTable(TableDef & tableDef) {
    name = tableDef.getName();
    tableInfo.fieldNumber = tableDef.getFieldsNum();
    
    pFieldStruct = (Field *)calloc(tableInfo.fieldNumber, sizeof(Field));
    if (pFieldStruct == NULL) {
        throw std::logic_error("Can not allocate memory.");
    }

    long recSize = 0;
    for (int i = 0; i < tableDef.getFieldsNum(); ++i) {
        Field curField;
        strcpy(curField.name, tableDef.getField(i).name.c_str());
        curField.type = tableDef.getField(i).type;

        if (curField.type == LONG) {
            curField.len = sizeof(long);
        } else {
            curField.len = tableDef.getField(i).len;
        }

        recSize += curField.len;
        pFieldStruct[i] = curField;
    }

    tableInfo.recordSize = recSize;
    tableInfo.dataOffset = sizeof(TableInfo) + tableInfo.fieldNumber * sizeof(Field);

    // table file creating
    if ((fd = open(name.c_str(), O_RDWR|O_CREAT|O_EXCL, 0666)) < 0) {
        throw std::logic_error("The table with this name has been already created.");
    }

    writeHeader();
    if (close(fd) < 0) {
        throw std::logic_error("Can not close the table file.");    
    }

    fd = -1;
}

void Table::insert(std::vector<char*> & newRecord) {
    if ((long)newRecord.size() != tableInfo.fieldNumber) {
        throw std::logic_error("Wrong new record fields number.");
    }

	Links links;
	links.nextOffset = 0;
	links.prevOffset = this -> tableInfo.lastRecordOffset;

	if (!links.prevOffset) {
		links.prevOffset = -1;
	}
    
    // getting position to write
    long curPos;
    if (tableInfo.firstDeletedOffset >= tableInfo.dataOffset) {
        curPos = tableInfo.firstDeletedOffset;
        if (lseek(fd, curPos, SEEK_SET) != curPos) {
            throw std::logic_error("Can not move to position in the file.");
        }
        if (read(fd, & links, sizeof(Links)) != sizeof(Links)) {
            throw std::logic_error("Can not read from the file.");
        }
        tableInfo.firstDeletedOffset = links.nextOffset;
    } else {
        curPos = tableInfo.dataOffset + tableInfo.totalRecordNumber * (tableInfo.recordSize + sizeof(Links));
        if (lseek(fd, curPos, SEEK_SET) != curPos) {
            throw std::logic_error("Can not move to position.");
        }
        ++(tableInfo.totalRecordNumber);
    }

    // writing new record
    if (lseek(fd, curPos, SEEK_SET) != curPos) {
        throw std::logic_error("Can not move to position in the file.");
    }

    if (write(fd, & links, sizeof(Links)) != sizeof(Links)) {
        throw std::logic_error("Can not write to file.");
    }

    for (long i = 0; i < tableInfo.fieldNumber; i++) {
        if (write(fd, newRecord[i], pFieldStruct[i].len) != pFieldStruct[i].len) {
            throw std::logic_error("Can not write to the file.");    
        }
    }
    
	modifyLinks(tableInfo.lastRecordOffset, curPos, LINK_NEXT);

	if (!tableInfo.recordNumber) {
		tableInfo.firstRecordOffset = curPos;
	}

	++(tableInfo.recordNumber);
	tableInfo.lastRecordOffset = curPos;

    writeHeader();
}

void Table::modifyLinks(long position, long value, enum Direction dir) {
	long posToWrite = 0;

	if (position != 0 && position != -1) {
		switch (dir) {
            case LINK_PREV:
                posToWrite = position;
                break;

		    case LINK_NEXT:
                posToWrite = position + sizeof(long);
                break;
            /*
            default:
                break;
            */
        }

	    if (lseek(fd, posToWrite, SEEK_SET) != posToWrite) {
            throw std::logic_error("Can not move to the position in the file.");
        }
	    if (write(fd, & value, sizeof(value)) != sizeof(value)) {
            throw std::logic_error("Can not write to the file.");
        }
    }
}

void Table::deleteTable(const string & tableName) {
    if (unlink(tableName.c_str())) {
        throw std::logic_error("Can not delete the table file.");
    }
}

void Table::openTable(const string & tableName) {
    name = tableName;

    if ((fd = open(name.c_str(), O_RDWR)) < 0) {
        throw std::logic_error("Can not open table file.");
    }
    
    readHeader();
    currentPos = tableInfo.firstRecordOffset;
}

void Table::closeTable() {
    if (close(fd) < 0) {
        throw std::logic_error("Can not close the table file.");
    }
    fd = -1;
}

void Table::writeHeader() {
    if (lseek(fd, 0, SEEK_SET) != 0) {
        throw std::logic_error("Can not move to position.");
    }
    
    if (write(fd, & tableInfo, sizeof(tableInfo)) != sizeof(tableInfo)) {
        throw std::logic_error("Can not write data.");
    }
    
    if (pFieldStruct) {
        if (write(fd, pFieldStruct, sizeof(Field) * tableInfo.fieldNumber) != (long)(sizeof(Field) * tableInfo.fieldNumber)) {
            throw std::logic_error("Can not write to the file.");
        }
    } else {
        throw std::logic_error("Can not write to the file.");    
    }
}

void Table::readHeader() {
    if (lseek(fd, 0, SEEK_SET) != 0) {
        throw std::logic_error("Can not move to position.");
    }

    if (read(fd, & tableInfo, sizeof(tableInfo)) != sizeof(tableInfo)) {
        throw std::logic_error("Can not read data.");
    }

    if (tableInfo.fieldNumber <= 0) {
        throw std::logic_error("Corrupted file.");
    }

    pFieldStruct = (Field*)calloc(tableInfo.fieldNumber, sizeof(Field));
    if (!pFieldStruct) {
        throw std::logic_error("Can not read from the file.");
    }

    if (read(fd, pFieldStruct, sizeof(Field) * tableInfo.fieldNumber) != (long)(sizeof(Field) * tableInfo.fieldNumber)) {
        throw std::logic_error("Can not write to the file.");
    }
}

void Table::movePrevious() {
    switch (currentPos) {
        case -1:
            throw std::logic_error("Bad position.");
        case 0:
            currentPos = tableInfo.lastRecordOffset;
            break;
        default:
            currentPos = links.prevOffset;
            break;
    }
}

void Table::moveNext() {
    switch (currentPos) {
        case -1:
            currentPos = tableInfo.firstRecordOffset;
            break;
        case 0:
            throw std::logic_error("Bad position.");
        default:
            currentPos = links.nextOffset;
            // cout << "current_pos: " << '\n'; // отладка
            break;
    }
}

void Table::getLong(const string & fieldName, long & value) {
    if (currentPos < tableInfo.dataOffset) {
        throw std::logic_error("Bad position.");
    }
    if (lseek(fd, currentPos, SEEK_SET) != currentPos) {
        throw std::logic_error("Can not move to the position.");
    }
    if (read(fd, & links, sizeof(Links)) != sizeof(Links)) {
        throw std::logic_error("Can not read the links from the file.");
    }

    if (lseek(fd, currentPos + sizeof(Links), SEEK_SET) != (long)(currentPos + sizeof(Links))) {
        throw std::logic_error("Can not move to the position in the file.");
    }

    char buf[64];
    for (long i = 0; i < tableInfo.fieldNumber; i++) {
        if (read(fd, buf, pFieldStruct[i].len) != pFieldStruct[i].len) {
            throw std::logic_error("Can not read from the file.");
        }
        if (!strcmp(fieldName.c_str(), pFieldStruct[i].name)) {
            if (pFieldStruct[i].type != LONG) {
                throw std::logic_error("Bad field type.");
            }
            memcpy(& value, buf, sizeof(long));
            return;
        }
    }

    throw std::logic_error("Field not found.");
}

void Table::getText(const string & fieldName, string & pvalue) {
    if (currentPos < tableInfo.dataOffset) {
        throw std::logic_error("Bad position.");
    }
    if (lseek(fd, currentPos, SEEK_SET) != currentPos) {
        throw std::logic_error("Can not move to the position.");
    }
    if (read(fd, & links, sizeof(Links)) != sizeof(Links)) {
        throw std::logic_error("Can not read the links from the file.");
    }

    if (lseek(fd, currentPos + sizeof(Links), SEEK_SET) != (long)(currentPos + sizeof(Links))) {
        throw std::logic_error("Can not move to the position in the file.");
    }
    
    char buf[64];
    memset(buf, 0, 64);
    for (long i = 0; i < tableInfo.fieldNumber; i++) {
        if (read(fd, buf, pFieldStruct[i].len) != pFieldStruct[i].len) {
            throw std::logic_error("Can not read from the file.");
        }
        if (!strcmp(fieldName.c_str(), pFieldStruct[i].name)) {
            if (pFieldStruct[i].type != TEXT) {
                throw std::logic_error("Bad field type.");
            }
        
            pvalue = string(buf);
            return;
        }
    }

    throw std::logic_error("Field not found.");
}

void Table::putLong(const string & fieldName, long pvalue) {
    if (currentPos < tableInfo.dataOffset) {
        throw std::logic_error("Bad position.");
    }
    if (lseek(fd, currentPos + sizeof(Links), SEEK_SET) != (long)(currentPos + sizeof(Links))) {
        throw std::logic_error("Can not move to the position in the file.");
    }

    char buf[64];
    memset(buf, 0, 64);

    for (long i = 0; i < tableInfo.fieldNumber; i++) {

        if (!strcmp(fieldName.c_str(), pFieldStruct[i].name)) {
            if (pFieldStruct[i].type != LONG) {
                throw std::logic_error("Bad field type.");
            }
            if (write(fd, & pvalue, sizeof(long)) < 0) {
                throw;
            }

            return;
        } else {
            if (read(fd, buf, pFieldStruct[i].len) < 0) {
                throw;
            }    
        }
    }

    throw std::logic_error("Field not found.");
}

void Table::putText(const string & fieldName, const string & pvalue) {
    if (currentPos < tableInfo.dataOffset) {
        throw std::logic_error("Bad position.");
    }
    if (lseek(fd, currentPos + sizeof(Links), SEEK_SET) != (long)(currentPos + sizeof(Links))) {
        throw std::logic_error("Can not move to the position in the file.");
    }

    char buf[64];
    memset(buf, 0, 64);

    for (long i = 0; i < tableInfo.fieldNumber; i++) {

        if (!strcmp(fieldName.c_str(), pFieldStruct[i].name)) {
            if (pFieldStruct[i].type != TEXT) {
                throw std::logic_error("Bad field type.");
            }

            if ((long)pvalue.size() > pFieldStruct[i].len) {
                throw std::logic_error("Field value is too long.");    
            }
            memset(buf, 0, pFieldStruct[i].len);
            strcpy(buf, pvalue.c_str());
            if (write(fd, buf, pFieldStruct[i].len) < 0) {
                throw;
            }

            return;
        } else {
            if (read(fd, buf, pFieldStruct[i].len) < 0) {
                throw;
            }
        }
    }

    throw std::logic_error("Field not found.");
}

void Table::deleteRec() {
    static unsigned long deleteMark = ~0;
    if (lseek(fd, currentPos, SEEK_SET) != currentPos) {
        throw std::logic_error("Can not move to the position in the file.");
    }
    if (write(fd, & deleteMark, sizeof(deleteMark)) != sizeof(deleteMark)) {
        throw std::logic_error("Can not write to the file.");
    }
    if (write(fd, & tableInfo.firstDeletedOffset, sizeof(tableInfo.firstDeletedOffset)) != sizeof(tableInfo.firstDeletedOffset)) {
        throw std::logic_error("Can not write to the file.");
    }
    tableInfo.firstDeletedOffset = currentPos;
    tableInfo.recordNumber--;

    if(links.prevOffset == -1) {
        tableInfo.firstRecordOffset = links.nextOffset;
        if (tableInfo.recordNumber == 0) {
            tableInfo.firstRecordOffset = -1;
            tableInfo.lastRecordOffset = 0;
        }
    } else {
        if (lseek(fd, links.prevOffset + sizeof(links.prevOffset), SEEK_SET) != (long)(links.prevOffset + sizeof(links.prevOffset))) {
            throw std::logic_error("Can not move to the position in the file.");
        }

        if (write(fd, & links.nextOffset, sizeof(links.nextOffset)) != sizeof(links.nextOffset)) {
            throw std::logic_error("Can not write to the file.");
        }
    }

    if (links.nextOffset == 0) {
        tableInfo.lastRecordOffset = links.prevOffset;
        if (tableInfo.recordNumber == 0) {
            tableInfo.firstRecordOffset = -1;
            tableInfo.lastRecordOffset = 0;
        }
    } else {
        if (lseek(fd, links.nextOffset, SEEK_SET) != links.nextOffset) {
            throw std::logic_error("Can not move to the position in the file.");
        }

        if(write(fd, & links.prevOffset, sizeof(links.prevOffset)) != sizeof(links.prevOffset)) {
            throw std::logic_error("Can not write to the file.");
        }
    }

    writeHeader();
}

int Table::getFieldLen(const string & fieldName) {
    long i = 0;
    while (i < tableInfo.fieldNumber) {
        if (!strcmp(fieldName.c_str(), pFieldStruct[i].name)) {
            return pFieldStruct[i].len;
        }
        ++i;
    }

    throw std::logic_error("Incorrect commmand: wrong field name.");
}

string Table::getFieldNameByIndex(int i) {
    return string(pFieldStruct[i].name);
}

long Table::getRecNum() {
    return tableInfo.recordNumber;
}

FieldType Table::getFieldType(string & fieldName) {
    long i = 0;
    while (i < tableInfo.fieldNumber) {
        if(!strcmp(fieldName.c_str(), pFieldStruct[i].name)) {
            return pFieldStruct[i].type;
        }
        ++i;
    }

    throw std::logic_error("Incorrect command: wrong field name.");
}

int Table::getFieldsNum() {
    return tableInfo.fieldNumber;
}