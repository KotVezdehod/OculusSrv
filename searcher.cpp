#include "searcher.h"
#include <QThread>
#include <QEventLoop>
#include <QDebug>
#include <QJsonDocument>
#include <filesystem>
#include "taskwatcher.h"
#include "httpserver.h"
#include "datetime.h"
#include "Oculus.h"

extern TaskWatcher *gTaskWatcher;
extern QVector<Task*> *gTasks;
extern std::mutex *gTasksListMutex;
extern QString *dataFolder;
extern unsigned long long startTime;
extern unsigned long long stopTime;
extern int threads;
extern std::mutex *mMutex;
namespace fs = std::filesystem;

Searcher::Searcher(QObject *parent)
    : QObject{parent}
{
}

void Searcher::process()
{

    emit started();

    timer = new QTimer();
    timer->connect(timer,SIGNAL(timeout()),this, SLOT(search()));
    timer->setSingleShot(false);
    timer->start(1000);
}


void Searcher::search()
{

    unsigned long long currentTime = DateTime().dateTimeToUlongLongTime(QDateTime::currentDateTime());

    if (currentTime < startTime) return ;
    if (currentTime > stopTime) return ;

    Sqlite sql;
    QString statementForDeletionLoc("SELECT id, dateTimeStamp from DataPool");
    QVector<QMap<QString, DataValue>> rowsForDeletion =
            sql.getRowsByStatement(&statementForDeletionLoc);

    for (DataRowsSet::Iterator it = rowsForDeletion.begin(); it!=rowsForDeletion.end(); std::advance(it,1))
    {

        DateTime dtCurrent;
        DateTime dtFromDB(it->value(DB_FIELDS::DATETIMESTAMP).intValue);
        unsigned long long difference = dtCurrent - dtFromDB;

        if (difference > 1000000)     //2 дней
        {
            sql.removeRowById(it->value(DB_FIELDS::ID).intValue);

            Diagnostics diagLoc(false, "Searcher::search: row removed by date: " +
                                it->value(DB_FIELDS::ID).strValue);

            diagLoc.throwLocalDiag();

            continue;
        }
    }

    QString strStatement("SELECT * FROM " + sql.TABLE_NAME + " WHERE status = 3");
    DataRowsSet rows = sql.getRowsByStatement(&strStatement);



    if (!rows.count())
        return;

    qDebug() << "Rows to recognizing: " << rows.length();

    for (DataRowsSet::Iterator it = rows.begin();
         it != rows.end(); std::advance(it,1))
    {

        QString fnLoc = it->value(DB_FIELDS::FILE).strValue;
        if (!fs::exists(fnLoc.toStdString())){
            qDebug() << "File " << fnLoc << " isn't exist. Removing row from database...";


//             if (!QFile::exists(it->value(DB_FIELDS::FILE).strValue))
//             {
                 QString delStatement = "DELETE FROM DataPool WHERE id = ";
                 delStatement += QString::number(it->value(DB_FIELDS::ID).intValue);

                 sql.execStatement(delStatement);
                 continue;
                 //Diagnostics(true, "FileCleaner::clean: удалена запись БД, указывающая на отсутствующий файл" + cIt->value("file").strValue).throwLocalDiag();
             //}
        }

        QString idLoc = QString::number(it->value(DB_FIELDS::ID).intValue);

        QString strStatement_0("UPDATE " + sql.TABLE_NAME + " " +
                             "SET status = 2 "
                             "WHERE id = " + idLoc);
        sql.execStatement(strStatement_0);

//        qDebug() << it->value(DB_FIELDS::PATTERN).strValue;
//        qDebug() << it->value(DB_FIELDS::FILE).strValue;
//        qDebug() << *dataFolder;

        QString t0("Starting analysing row with id = " + idLoc);
        t0 += "...";
        Diagnostics(true,  t0).throwLocalDiag();
        qDebug() << t0;

//        std::vector<std::string> errDescr;
//        std::vector<std::string> errLevel;
        bool result = false;

        QString pat = it->value(DB_FIELDS::PATTERN).strValue;
        QString file = it->value(DB_FIELDS::FILE).strValue;
        std::string resultDescr;

        tryFindPattern(
                    pat.toStdString(),
                    result,
                    file.toStdString(),
                    dataFolder->toStdString(),
                    resultDescr,
                    threads);

//        if (!errDescr.empty()){
//            QJsonArray jArrOut;
//            for (std::vector<std::string>::const_iterator it = errDescr.cbegin(); it != errDescr.cend(); std::advance(it,1)){
//                jArrOut.append(QJsonValue(QString::fromStdString(*it)));
//            }
//            QJsonDocument docOut(jArrOut);
//            QString t = docOut.toJson();
//            Diagnostics(false, t);
//            qDebug() << t;
//        } else {
            QString strStatement_1("UPDATE " + sql.TABLE_NAME + " " +
                                 "SET status = " + QString::number(result) + " "
                                 "WHERE id = " + idLoc);
            sql.execStatement(strStatement_1);

            //qDebug() << QString::fromStdString(resultDescr);

            fs::path logPath(dataFolder->toStdString());
            logPath.append("log.txt");
            std::ofstream ofs;
            std::lock_guard<std::mutex> mute(*mMutex);

            ofs.open(logPath,std::ios::app);
            if (ofs.is_open()){
                ofs << resultDescr << std::endl;
                ofs.close();
            }

//            QString t1("Result analysing row with id = " + idLoc);
//            t1 += " is ";
//            t1 += QString::number(result);
//            Diagnostics(true,  t1).throwLocalDiag();
//            qDebug() << t1;
        //}

    }

    return;
}
