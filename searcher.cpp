#include "searcher.h"
#include <QThread>
#include <QEventLoop>
#include <QDebug>
#include <QJsonDocument>
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

        if (difference > 691200000)     //8 ����
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


    qDebug() << "Rows to recognizing: " << rows.length();
    if (!rows.count())
        return;

    for (DataRowsSet::Iterator it = rows.begin();
         it != rows.end(); std::advance(it,1))
    {

        QString idLoc = QString::number(it->value(DB_FIELDS::ID).intValue);

        QString strStatement("UPDATE " + sql.TABLE_NAME + " " +
                             "SET status = 2 "
                             "WHERE id = " + idLoc);
        sql.execStatement(strStatement);

//        qDebug() << it->value(DB_FIELDS::PATTERN).strValue;
//        qDebug() << it->value(DB_FIELDS::FILE).strValue;
//        qDebug() << *dataFolder;

        QString t("Starting analysing row with id = " + idLoc);
        t += "...";
        Diagnostics(true,  t).throwLocalDiag();
        qDebug() << t;

        std::vector<std::string> errDescr;
        std::vector<std::string> errLevel;
        bool result = false;


        tryFindPattern(
                    it->value(DB_FIELDS::PATTERN).strValue.toStdString(),
                    result,
                    it->value(DB_FIELDS::FILE).strValue.toStdString(),
                    dataFolder->toStdString(),
                    errDescr,
                    errLevel);

        if (!errDescr.empty()){
            QJsonArray jArrOut;
            for (std::vector<std::string>::const_iterator it = errDescr.cbegin(); it != errDescr.cend(); std::advance(it,1)){
                jArrOut.append(QJsonValue(QString::fromStdString(*it)));
            }
            QJsonDocument docOut(jArrOut);
            QString t = docOut.toJson();
            Diagnostics(false, t);
            qDebug() << t;
        } else {
            QString strStatement("UPDATE " + sql.TABLE_NAME + " " +
                                 "SET status = " + QString::number(result) + " "
                                 "WHERE id = " + idLoc);
            sql.execStatement(strStatement);

            QString t("Result analysing row with id = " + idLoc);
            t += " is ";
            t += QString::number(result);
            Diagnostics(true,  t).throwLocalDiag();
            qDebug() << t;
        }

    }

    return;
}
