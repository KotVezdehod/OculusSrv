#include "sqlite.h"
#include <QThread>
#include <QUuid>
#include <QJsonObject>
#include "datetime.h"

extern QString *dataFolder;

//========================= Sqlite
Sqlite::Sqlite()
{
    dbName_ = SqliteSupport().generateObjectThreadDbName();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbName_);
    db.setDatabaseName((*dataFolder) + "\\CommonDb.sqlite");
    db.open();

    checkDbStructure();
}

Sqlite::~Sqlite()
{
    {
        QSqlDatabase db = QSqlDatabase::database(dbName_);
        if (db.isOpen())
            db.close();
    }

    QSqlDatabase::removeDatabase(dbName_);
}

Diagnostics Sqlite::init()
{
    Diagnostics diag;
    create();
    return diag;
}

void Sqlite::checkDbStructure()
{

    QVector<QString> qVecRet;

    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery query(conn);
    QString st = "PRAGMA table_info(" + TABLE_NAME + ")";

    QString nativeError;

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool queryStatus = query.exec(st);
    lg.reset();

    if (!queryStatus)
    {
        nativeError = query.lastError().nativeErrorCode();
    }
    if ((!queryStatus)&&nativeError.compare("5"))            //база может быть занята.
    {
        return;
    }

    QSqlRecord rec = query.record();
    while (query.next())
    {
        qVecRet.push_back(query.value(rec.indexOf("name")).toString());
    }

    if (qVecRet.size()==0)
    {
        Diagnostics diag(false, "Sqlite::checkDbStructure: database recreated", 200, DebugLevels::error());
        diag.throwLocalDiag();
        create();
        return;
    }

    //Если в структуре появились новые колонки, то добавляем их
    T_Column dbStructure = DbStructure();
    for (T_Column::Iterator it = dbStructure.begin(); it!=dbStructure.end(); std::advance(it, 1))
    {
        if (!qVecRet.contains(it.key()))
        {
            execStatement("ALTER TABLE " + TABLE_NAME + " ADD " + it.key() + " " + SqliteSupport::dataType(it->dataType));
        }
    }

    return;
};

Diagnostics Sqlite::execStatement(QString inStatement)
{

    Diagnostics diag;
    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery query(conn);
    bool unknownFieldTypeErrReported = false;

    QJsonArray rootArray;

    QString fieldName;
    SqliteSupport::SQLITE_DATATYPES dataType;
    SqliteSupport support;

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool qResult = query.exec(inStatement);
    lg.reset();

    if (qResult)
    {
        QSqlRecord rec = query.record();
        while (query.next())
        {
            QJsonObject row;
            for (int j = 0; j<rec.count(); j++)
            {


                dataType = support.getFieldDataType(rec.field(j).metaType());
                fieldName = rec.field(j).name();

                switch (dataType)
                {
                case SqliteSupport::SQLITE_DATATYPES::INTEGER:
                {
                    unsigned long long valueLoc = query.value(rec.indexOf(fieldName)).toULongLong();
                    row.insert(fieldName, QJsonValue::fromVariant(valueLoc));
                }
                    break;
                case SqliteSupport::SQLITE_DATATYPES::TEXT:
                {
                    QString valueLoc = query.value(rec.indexOf(fieldName)).toString();
                    row.insert(fieldName, valueLoc);
                }
                    break;
                case SqliteSupport::SQLITE_DATATYPES::REAL:
                {
                    double valueLoc = query.value(rec.indexOf(fieldName)).toDouble();
                    row.insert(fieldName, QJsonValue::fromVariant(valueLoc));
                }
                    break;
                case SqliteSupport::SQLITE_DATATYPES::BLOB:
                {
                    QString valueLoc(query.value(rec.indexOf(fieldName)).toByteArray().toBase64());
                    row.insert(fieldName, valueLoc);
                }
                    break;

                default:
                {
                    if (!unknownFieldTypeErrReported)
                    {
                        unknownFieldTypeErrReported = true;
                        Diagnostics(false, "Sqlite::execStatement: unknown metatipe received from database, request: " +
                                    fieldName).throwLocalDiag();
                    }
                }
                    break;
                }
            }

            rootArray.append(row);

        }
        QJsonDocument jDoc(rootArray);
        diag.obj = jDoc.toJson();
    }
    else
    {
        QString descriptionLoc = "Sqlite::execStatement: unable to execute (";
        descriptionLoc += support.buildDiagString(&conn, &query);
        descriptionLoc += ")";
        diag.status = false;
        diag.description = descriptionLoc;
        diag.throwLocalDiag();
    }

    return diag;

}


QString Sqlite::getCreateStatement()
{
    T_Column dbStruc = DbStructure();

    QString ret("CREATE TABLE IF NOT EXISTS " + TABLE_NAME + " (");
    ret += "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE";

    for (T_Column::iterator it = dbStruc.begin(); it!=dbStruc.end(); std::advance(it,1))
    {
        ret+=QString(", %1 %2").replace("%1",it.key()).replace("%2", SqliteSupport::dataType(it.value().dataType));
    }
    ret+=");";

    return ret;

}

Diagnostics Sqlite::create()
{
    Diagnostics diag;
    QSqlDatabase conn = QSqlDatabase::database(dbName_);

    QSqlQuery a_query(conn);
    // DDL query
    QString str = getCreateStatement();

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool b = a_query.exec(str);
    lg.reset();

    if (!b)
    {
        SqliteSupport support;
        QString descriptionLoc = "Sqlite::create: unable to create database (";
        descriptionLoc += support.buildDiagString(&conn, &a_query);
        descriptionLoc += ")";
        diag.status = false;
        diag.description = descriptionLoc;
        diag.throwLocalDiag();

    }

    QString stAddColumnsBase = "ALTER TABLE " + TABLE_NAME;
    stAddColumnsBase += " ADD ";

    T_Column struc = DbStructure();

    SqliteSupport support;
    for (T_Column::Iterator it = struc.begin(); it!=struc.end(); std::advance(it, 1))
    {
        QString stAddColumnsFull(stAddColumnsBase);
        stAddColumnsFull += it.key() + " ";
        stAddColumnsFull += SqliteSupport::dataType(it->dataType) + ";";

        USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
        bool qResult = a_query.exec(stAddColumnsFull);
        lg.reset();

        if (qResult)
        {
            QString descriptionLoc = "Sqlite::create: new column created: ";
            descriptionLoc += it.key();
            diag.status = true;
            diag.description = descriptionLoc;
            diag.throwLocalDiag();

        }
        else
        {
            QString descriptionLoc = "Sqlite::create: unable to execute (";
            descriptionLoc += support.buildDiagString(&conn, &a_query);
            descriptionLoc += ")";
            diag.status = false;
            diag.description = descriptionLoc;
            diag.throwLocalDiag();

        }
    }

    return diag;
}

Diagnostics Sqlite::clear()
{
    Diagnostics diag;
    SqliteSupport support;

    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery a_query(conn);
    // DDL query

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool b = a_query.exec("DELETE FROM " + TABLE_NAME);
    lg.reset();

    if (!b)
    {

        QString descriptionLoc = "Sqlite::clear: unable to execute (";
        descriptionLoc += support.buildDiagString(&conn, &a_query);
        descriptionLoc += ")";
        diag.status = false;
        diag.description = descriptionLoc;

        diag.throwLocalDiag();
    }
    return diag;
}

Diagnostics Sqlite::drop()
{
    Diagnostics diag;
    SqliteSupport support;
    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery a_query(conn);
    // DDL query

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool b = a_query.exec("DROP TABLE " + TABLE_NAME);
    lg.reset();

    if (!b)
    {

        QString descriptionLoc = "Sqlite::dropTable: unable to execute (";
        descriptionLoc += support.buildDiagString(&conn, &a_query);
        descriptionLoc += ")";
        diag.status = false;
        diag.description = descriptionLoc;

        diag.throwLocalDiag();
    }
    return diag;
}

Diagnostics Sqlite::insertRow(DbStructure *valuesIn)
{
    Diagnostics diag = Diagnostics();
    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery query(conn);
    SqliteSupport support;

    if (!valuesIn->value(DB_FIELDS::DATETIMESTAMP).intValue){
        valuesIn->setValue(DB_FIELDS::DATETIMESTAMP, DateTime().currentDateTime);
    }

    QString stLoc = "SELECT * FROM " + TABLE_NAME;
    QVector<T_Column> rows = getRowsByStatement(&stLoc);

    // DDL query
    T_Column struc = DbStructure();
    QString str1;
    QString str2;
    str1 = "INSERT INTO ";
    str1 += TABLE_NAME + " (";
    str2 = "VALUES (";

    bool firstLoc = true;

    for (T_Column::iterator it = valuesIn->begin();
         it != valuesIn->end();
         std::advance(it, 1))
    {
        if (!struc.contains(it.key()))
            continue;
        if (!firstLoc)
        {
            str1 += ", ";
            str2 += ", ";
        }
        str1 +=  it.key();
        str2 += ":";
        str2 += it.key();
        firstLoc = false;
    }

    str1 += ")";
    str2 += ");";
    str1 += " ";
    str1 += str2;

    if (!query.prepare(str1))
    {
        QString descriptionLoc = "Sqlite::insertRow: unable to execute prepare (";
        descriptionLoc += support.buildDiagString(&conn, &query);
        descriptionLoc += ")";
        diag.status = false;
        diag.description = descriptionLoc;

        diag.throwLocalDiag();
    }
    else
    {
        for (typename T_Column::iterator it = valuesIn->begin();
             it != valuesIn->end();
             std::advance(it, 1))
        {
            if (!struc.contains(it.key()))
                continue;

            switch (it.value().dataType)
            {
            case SqliteSupport::SQLITE_DATATYPES::TEXT:
                query.bindValue(":" + it.key(), it.value().strValue);
                break;
            case SqliteSupport::SQLITE_DATATYPES::INTEGER:
                query.bindValue(":" + it.key(), it.value().intValue);
                break;
            case SqliteSupport::SQLITE_DATATYPES::REAL:
                query.bindValue(":" + it.key(), it.value().realValue);
                break;
            default:
                break;
            }
        }

        USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
        bool qResult = query.exec();
        lg.reset();

        if(!qResult)
        {
            QString descriptionLoc = "Sqlite::insertRow: unable to execute (";
            descriptionLoc += support.buildDiagString(&conn, &query);
            descriptionLoc += ")";
            diag.status = false;
            diag.description = descriptionLoc;

            diag.throwLocalDiag();

        }
    }
    return diag;
}

Diagnostics Sqlite::removeRowById(int idIn)
{
    Diagnostics diag;
    SqliteSupport support;

    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery a_query(conn);
    // DDL query

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool b = a_query.exec("DELETE FROM " + TABLE_NAME + " WHERE id = " + QString::number(idIn));
    lg.reset();

    if (!b)
    {
        QString descriptionLoc = "Sqlite::removeRowById: unable to execute (";
        descriptionLoc += support.buildDiagString(&conn, &a_query);
        descriptionLoc += ")";
        diag.status = false;
        diag.description = descriptionLoc;

        diag.throwLocalDiag();
    }
    return diag;
}

DataRowsSet Sqlite::getRowsByDate(unsigned long long startDateIn, unsigned long long endDateIn)
{
    DataRowsSet qVecRet;
    SqliteSupport support;

    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery query(conn);
    QString st = "SELECT * FROM " + TABLE_NAME + " WHERE dateTimeStamp >= " + QString::number(startDateIn);
    st+= " AND ";
    st+= "dateTimeStamp <= " + QString::number(endDateIn);

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool qResult = query.exec(st);
    lg.reset();

    if (!qResult)
    {

        QString descriptionLoc = "Sqlite::getRowsByDate: unable to execute " + st + " (";
        descriptionLoc += support.buildDiagString(&conn, &query);
        descriptionLoc += query.lastError().text();
        descriptionLoc += ")";
        Diagnostics diag(false, descriptionLoc);

        diag.throwLocalDiag();
    }
    else
    {
        createOutDataVector(&qVecRet, &query);
    }

    return qVecRet;
}

DataRowsSet Sqlite::getRowsByStatement(QString *inStatement)
{
    DataRowsSet qVecRet;

    SqliteSupport support;
    QSqlDatabase conn = QSqlDatabase::database(dbName_);
    QSqlQuery query(conn);

    QString st = (*inStatement);

    USqlLockGuard lg = std::make_unique<LockGuard>(*mSqlMutex);
    bool qResult = query.exec(st);
    lg.reset();

    if (!qResult)
    {

        QString descriptionLoc = "Sqlite::getRowsByStatement: unable to execute " + st + " (";
        descriptionLoc += support.buildDiagString(&conn, &query);
        descriptionLoc += query.lastError().text();
        descriptionLoc += ")";
        Diagnostics diag(false, descriptionLoc);
        diag.throwLocalDiag();
    }
    else
    {
        createOutDataVector(&qVecRet, &query);
    }

    return qVecRet;
}

void Sqlite::createOutDataVector(DataRowsSet *dataVectorRef, QSqlQuery* query)
{
    QSqlRecord rec = query->record();
    T_Column struc = DbStructure();
    struc.insert("id", DataValue(SqliteSupport::SQLITE_DATATYPES::INTEGER,"",0,0));

    SqliteSupport::SQLITE_DATATYPES dataType;
    QString fieldName;

    while (query->next())
    {
        dataVectorRef->push_back(T_Column());
        T_Column *ref = &(*dataVectorRef)[dataVectorRef->size()-1];

        for (int j = 0 ;j<rec.count(); j++)
        {
            dataType = SqliteSupport::getFieldDataType(rec.field(j).metaType());
            fieldName = rec.fieldName(j);

            switch (dataType)
            {
            case SqliteSupport::SQLITE_DATATYPES::TEXT:
            {
                ref->insert(fieldName, DataValue(dataType, query->value(j).toString(), 0, 0));
            }
                break;
            case SqliteSupport::SQLITE_DATATYPES::INTEGER:
            {
                ref->insert(fieldName, DataValue(dataType, "", query->value(j).toULongLong(), 0));
            }
                break;
            case SqliteSupport::SQLITE_DATATYPES::REAL:
            {
                ref->insert(fieldName, DataValue(dataType, "", 0, query->value(j).toDouble()));
            }
                break;
            default:
                break;
            }
        }
    }
}

//====================== SqliteSupport

QString SqliteSupport::generateObjectThreadDbName()
{
    QString guid = QUuid::createUuid().toString();
    return QString("%1").arg((quintptr)QThread::currentThreadId(), QT_POINTER_SIZE*2, 16, QChar('0')) + guid;
}

QString SqliteSupport::buildDiagString(QSqlDatabase *conn, QSqlQuery *query)
{
    QString out = "db_err: " + conn->lastError().text();
    if (query)
    {
        out+="; ";
        out+="query_err_code: " + query->lastError().nativeErrorCode() + "; ";
        out+="query_err_msg: " + query->lastError().driverText();
        out+=", " + query->lastError().databaseText();
        out+=", " + query->lastError().text();
    }
    return out;
}

