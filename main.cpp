//toDo:
//х.Проверка на повторный запуск

#include <QCoreApplication>
#include <iostream>
#include <filesystem>
//#include <QEventLoop>
#include "Oculus.h"
#include "sqlite.h"
#include "diagnostics.h"
#include "taskwatcher.h"
#include "httpserver.h"
#include "searcher.h"
#include "filecleaner.h"
#include "datetime.h"

namespace fs = std::filesystem;

QString *dataFolder = new QString();
QVector<Task*> *gTasks = nullptr;
TaskWatcher *gTaskWatcher = nullptr;
std::mutex *gTasksListMutex = new std::mutex;
std::mutex *gFileWritingMutex = new std::mutex;
FileCleaner *gFileCleaner = nullptr;
unsigned long long startTime = DateTime().dateTimeToUlongLongTime(QDateTime::fromString("20000101000000","yyyyMMddHHmmss"));
unsigned long long stopTime = DateTime().dateTimeToUlongLongTime(QDateTime::fromString("20000101080000","yyyyMMddHHmmss"));
int threads = 4;

std::ofstream ofLck;

int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);

    std::string strDataFolder;

    for (int i = 0; i != argc; i++) {
        if (!strcmp(argv[i], "-data")) {
            i++;
            if (i >= argc)
                break;
            strDataFolder.assign(argv[i]);
        }

        if (!strcmp(argv[i], "-stime")) {
            i++;
            if (i >= argc)
                break;
            unsigned long long startTimeLoc = QString(argv[i]).toULongLong();
            if (startTimeLoc > 235959){
                QString t = "Start time invalid ('-stime' parameter)!";
                qDebug()<< t;
                Diagnostics(false,t).throwLocalDiag();
                return -1;
            }
            startTime = startTimeLoc;
        }
        if (!strcmp(argv[i], "-ftime")) {
            i++;
            if (i >= argc)
                break;

            unsigned long long stopTimeLoc = QString(argv[i]).toULongLong();
            if (stopTimeLoc > 235959){
                QString t = "Fin time invalid ('-ftime' parameter)!";
                qDebug()<< t;
                Diagnostics(false,t).throwLocalDiag();
                return -1;
            }
            stopTime = stopTimeLoc;
        }

        if (!strcmp(argv[i], "-threads")) {
            i++;
            if (i >= argc)
                break;

            threads = QString(argv[i]).toInt();
        }
    }

//    if (startTime >= stopTime){
//        QString t = "Start can't be less then stoptime!";
//        qDebug()<< t;
//        Diagnostics(false,t).throwLocalDiag();
//        return -1;
//    }


    if (strDataFolder.empty()){
        QString t = "No datapath defined ('-data' parameter)!";
        qDebug()<< t;
        Diagnostics(false,t).throwLocalDiag();
        return -1;
    };

    *dataFolder = QString::fromStdString(strDataFolder);


    fs::path lckFilePath(dataFolder->toStdString());
    lckFilePath.append("lck");

    ofLck.open(lckFilePath, std::fstream::app, _SH_DENYRW);
    if (!ofLck.is_open()){
        QString t = "Another process Oculus is running. Shutting down.";
        Diagnostics(false,t).throwLocalDiag();
        qDebug() << t;
        return -1;
    }


    gTasks = new QVector<Task*>();
    gTaskWatcher = new TaskWatcher();

    if(!QDir(*dataFolder).exists()){
        QString t = "Database folder isn't exists.";
        Diagnostics(false,t).throwLocalDiag();
        qDebug() << t;
        return -1;
    }

    Sqlite sql;

    Diagnostics diagRes = sql.execStatement("SELECT id from " + sql.TABLE_NAME + " limit 1");
    if (!diagRes.status){
        diagRes.throwLocalDiag();
        qDebug() << diagRes.description;
        return -1;
    };

    DataRowsSet rows = sql.getRowsByOneParameter(DB_FIELDS::STATUS, 2);
    for (auto &row: rows) {

        QString strStatement("UPDATE " + sql.TABLE_NAME + " " +
                                                  "SET status = 0 "
                                                  "WHERE id = " + QString::number(row.value(DB_FIELDS::ID).intValue));
        sql.getRowsByStatement(&strStatement);
    }


    //=== task watcher
    std::unique_ptr<Task> T1(new Task(gTaskWatcher, "TaskWatcher"));
    T1->start();

    //=== Start http server
    HttpServer *httpserver = new HttpServer();
    Task* tHttpServer = new Task(httpserver,"HttpServer");
    QObject::connect(httpserver, SIGNAL(started()), gTaskWatcher, SLOT(localServerStarted()));
    tHttpServer->start();


    //=== Start searcher
    Searcher *searcher = new Searcher();
    Task* tSearcher = new Task(searcher, "Searcher");
    QObject::connect(searcher, SIGNAL(started()), gTaskWatcher, SLOT(searcherStarted()));
    tSearcher->start();


    //=== FileCleaner
    gFileCleaner = new FileCleaner();
    Task *tFileCleaner = new Task(gFileCleaner,"FileCleaner");
    QObject::connect(gFileCleaner, SIGNAL(started()), gTaskWatcher, SLOT(fileCleanerStarted()));
    tFileCleaner->start();


//    std::string imgFilePathName("C:\\C++\\Oculus\\v8_6583_3a0c.png");
//    std::string ocrDataPath("C:\\C++\\Oculus");
//    std::vector<std::string> errDescr;
//    std::vector<std::string> errLevel;
//    bool result = false;
//    tryFindPattern("200438020246", result, imgFilePathName, ocrDataPath, errDescr, errLevel);
//    std::cout << imgFilePathName << " status:" << result << std::endl;


//    QEventLoop evLoop;
//    evLoop.exec();

    return a.exec();
}
