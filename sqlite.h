#ifndef SQLITE_H
#define SQLITE_H
#include "diagnostics.h"
#include <QMap>
#include <QtSql>
#include <QString>


namespace DB_FIELDS
{
    constexpr static char ID[]              = "id";
    //constexpr static char TABLE_NAME[]      = "$tableName$";
    constexpr static char FILE[]            = "file";
    constexpr static char FILENAME[]        = "fileName";
    constexpr static char STATUS[]          = "status";
    constexpr static char DATETIMESTAMP[]   = "dateTimeStamp";
    constexpr static char PATTERN[]         = "pattern";
};


enum STATUSES
{
    readyToSend,
    sendingNow,
    beenSent,
    error
};

static QString ROW_STATUS_NAME(STATUSES statusIn)
{
    switch (statusIn) {
    case readyToSend:
        return "к_отправке";
    case sendingNow:
        return "идет_отправка";
    case error:
        return "ошибка";
    case beenSent:
        return "отправлено";
    default:
        return "";
        break;
    }
}

class DbStructure;

class SqliteSupport
{

public:
    SqliteSupport(){};
    ~SqliteSupport(){};

    enum SQLITE_DATATYPES
    {
        ISNULL,
        INTEGER,
        REAL,
        TEXT,
        BLOB,
        DATETIME
    };

    static QString dataType(SQLITE_DATATYPES dtIn)
    {

        switch (dtIn)
        {
        case TEXT:
            return "TEXT";
        case INTEGER:
            return "INTEGER";
        case REAL:
            return "REAL";
        case BLOB:
            return "BLOB";
        case DATETIME:
            return "DATETIME";
        case ISNULL:
            return "NULL";
        default:
            break;
        }
        return "";
    }

    QString buildDiagString(QSqlDatabase *conn, QSqlQuery *query = nullptr);
    QString generateObjectThreadDbName();

    static SqliteSupport::SQLITE_DATATYPES getFieldDataType(QMetaType metaType)
    {

        if (metaType == QMetaType(QMetaType::Int))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::Bool))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::UInt))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::Long))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::LongLong))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::Short))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::ULong))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::ULongLong))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::UShort))
        {
            return SQLITE_DATATYPES::INTEGER;
        }
        else if (metaType == QMetaType(QMetaType::Double))
        {
            return SQLITE_DATATYPES::REAL;
        }
        else if (metaType == QMetaType(QMetaType::Float))
        {
            return SQLITE_DATATYPES::REAL;
        }
        else if (metaType == QMetaType(QMetaType::QChar))
        {
            return SQLITE_DATATYPES::TEXT;
        }
        else if (metaType == QMetaType(QMetaType::QString))
        {
            return SQLITE_DATATYPES::TEXT;
        }
        else if (metaType == QMetaType(QMetaType::Char))
        {
            return SQLITE_DATATYPES::TEXT;
        }
        else if (metaType == QMetaType(QMetaType::Char16))
        {
            return SQLITE_DATATYPES::TEXT;
        }
        else if (metaType == QMetaType(QMetaType::Char32))
        {
            return SQLITE_DATATYPES::TEXT;
        }
        else if (metaType == QMetaType(QMetaType::QByteArray))
        {
            return SQLITE_DATATYPES::BLOB;
        }
        else if (metaType == QMetaType(QMetaType::QByteArray))
        {
            return SQLITE_DATATYPES::BLOB;
        }
        else
        {
            return SQLITE_DATATYPES::ISNULL;
        }
    }

};

class DataValue
{
public:

    DataValue(SqliteSupport::SQLITE_DATATYPES dataTypeIn = SqliteSupport::SQLITE_DATATYPES::INTEGER, QString strValueIn = "", unsigned long long intValueIn = 0, double realValueIn = 0):
        dataType(dataTypeIn), strValue(strValueIn), intValue(intValueIn), realValue(realValueIn){}

    SqliteSupport::SQLITE_DATATYPES dataType;
    QString strValue;
    unsigned long long intValue = 0;
    double realValue = 0;
};

typedef QMap<QString, DataValue> T_Column;
typedef QList<T_Column> DataRowsSet;

class Sqlite
{


private:
    inline QString generateObjectThreadDbName();

public:

    QString TABLE_NAME = "DataPool";

    enum DATA_FLOW_DIRECTION
    {
        toBackEnd,
        toLocalApplication
    };

public:

    Sqlite();
    ~Sqlite();

    Diagnostics init();
    //QString genConnectionName();
    Diagnostics execStatement(QString inStatement);


    //QMap<QString, DataValue> getDbStructure();
    QString getCreateStatement();
    Diagnostics clear();
    Diagnostics create();
    Diagnostics drop();

    Diagnostics insertRow(DbStructure *valuesIn);
    Diagnostics removeRowById(int idIn);

    //Qt не умеет шаблонизировать функции при их разнесении по 2м файлам - приходится реализацию в заголовочном делать
    template<class T>
    DataRowsSet getRowsByOneParameter(QString paNameIn, T parValueIn, int count = 0, bool orderById = false)
    {
        DataRowsSet qVecRet;

        //Sqlite sqlite;
        QSqlDatabase conn = QSqlDatabase::database(dbName_);

        QSqlQuery query(conn);
        QString st = "SELECT * FROM " + TABLE_NAME + " WHERE " + paNameIn + " = ";

        if constexpr(std::is_same<T, int>::value)
        {
            st += QString::number(static_cast<int>(parValueIn));
        }
        else if constexpr(std::is_same<T, QString>::value)
        {
            st += "'";
            st += static_cast<QString>(parValueIn);
            st += "'";
        };

        if (count > 0)
        {
            st+=" LIMIT ";
            st+= QString::number(count);
        };

        if (orderById)
        {
            st+=" ORDER BY id";
        };

        SqliteSupport support;

        if (!query.exec(st))
        {
            QString descriptionLoc = "Sqlite::getRowsByOneParameter: unable to execute " + st + " (";
            descriptionLoc += support.buildDiagString(&conn, &query);
            descriptionLoc += ")";
            Diagnostics diag(false, descriptionLoc);
            diag.throwLocalDiag();
        }
        else
        {
            createOutDataVector(&qVecRet, &query);
        }

        return qVecRet;
    };
    DataRowsSet getRowsByDate(unsigned long long startDateIn, unsigned long long endDateIn);
    DataRowsSet getRowsByStatement(QString *inStatement);


private:
    void createOutDataVector(DataRowsSet *dataVectorRef, QSqlQuery* query);
    void checkDbStructure();
    QString dbName_;

};

class DbStructure: public T_Column
{

public:

    DbStructure() {
            this->insert(QString(DB_FIELDS::STATUS), DataValue(SqliteSupport::SQLITE_DATATYPES::INTEGER));
            this->insert(QString(DB_FIELDS::FILE), DataValue(SqliteSupport::SQLITE_DATATYPES::TEXT));
            this->insert(QString(DB_FIELDS::PATTERN), DataValue(SqliteSupport::SQLITE_DATATYPES::TEXT));
            this->insert(QString(DB_FIELDS::FILENAME), DataValue(SqliteSupport::SQLITE_DATATYPES::TEXT));
            this->insert(QString(DB_FIELDS::DATETIMESTAMP), DataValue(SqliteSupport::SQLITE_DATATYPES::INTEGER));
    }

    template <typename T>
    void setValue(const char *fieldIn, T valIn){

        QString fieldLoc(fieldIn);
        if constexpr (std::is_same<T, QString>::value){
            T_Column::Iterator it = find(fieldLoc);
            if (it!=this->end()){
                it->strValue = valIn;
            };
        }
        else if constexpr (std::is_same<T, double>::value){
            T_Column::Iterator it = find(fieldLoc);

            if (it!=this->end()){
                it->realValue = valIn;
            }
        }
        else {          //other (if it cant be casted to ulong, then - error)
            unsigned long long ulongLoc = static_cast<T>(valIn);
            T_Column::Iterator it = find(fieldLoc);
            if (it!=this->end()){
                it->intValue = ulongLoc;
            }
        }
    }
};

#endif // SQLITE_H
