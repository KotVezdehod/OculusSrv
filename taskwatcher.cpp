#include "taskwatcher.h"
#include <QDebug>
#include <QtConcurrent>
#include <datetime.h>
#include "diagnostics.h"
//#include "http.h"
#include "searcher.h"

extern QVector<Task*> *gTasks;
extern TaskWatcher* gTaskWatcher;
extern std::mutex *gTasksListMutex;

//======================== TaskWatcher
TaskWatcher::TaskWatcher(QObject *parent)
    : QObject{parent}{}

void TaskWatcher::process()
{
    Diagnostics(true, "TaskWatcher started");

    timer = new QTimer();
    timer->setSingleShot(false);
    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(watch()));
    timer->start(5000);
}

void TaskWatcher::watch()
{

    QVector<Task*>::Iterator it = std::find_if(gTasks->begin(), gTasks->end(), [](Task* t)
    {
        return !t->description.compare("Searcher");
    });

    bool isSearcherAlive = it!=gTasks->end();

    //qDebug() << "======= Tasks remains: " << QString::number(gTasks->size());

    if (!isSearcherAlive && searcherWorking)
    {
        Diagnostics(false, "======= Searcher's thread died. Reloading...").throwLocalDiag();

        Searcher *searcher = new Searcher();
        Task* tSearcher = new Task(searcher,"Searcher");
        QObject::connect(searcher, SIGNAL(started()), gTaskWatcher, SLOT(searcherWorking()));
        tSearcher->start();
    }

}

void TaskWatcher::objectDestroyed(QObject *obj)
{
        std::lock_guard<std::mutex> mutexLoc(*gTasksListMutex);

        gTasks->erase(std::remove_if(gTasks->begin(), gTasks->end(), [obj](Task* t)
        {
            if (t->object == obj)
            {
                qDebug()<< "================== Task was excluded from list: " << t->description;
                return true;
            }
            else
            {
                return false;
            };

        }), gTasks->end());
}

void TaskWatcher::localServerStarted()
{
    localServerListening = true;
    Diagnostics(true, "HttpServer started");
    return;
}

void TaskWatcher::searcherStarted()
{
    searcherWorking = true;
    Diagnostics(true, "Searcher started");
    return;
}

void TaskWatcher::fileCleanerStarted()
{
    Diagnostics(true, "FileCleaner started");
    return;
}

void Task::start()
{
    std::lock_guard<std::mutex> mLoc(*gTasksListMutex);
    thread = new QThread();
    gTasks->push_back(this);
    object->moveToThread(thread);

    QObject::connect(thread, SIGNAL(started()),    object, SLOT(process()));        //чтобы при старте потока - запустился на исполнение код
    QObject::connect(thread, SIGNAL(finished()),   object, SLOT(deleteLater()));    //чтобы при завершении потока удалился объект, код из которого выполняется.
    QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));      //чтобы при завершении потока удалился объект самого потока
    QObject::connect(object, SIGNAL(destroyed(QObject*)), gTaskWatcher, SLOT(objectDestroyed(QObject*)));       //чтобы оповестить наш функционал о том что объект и поток больше не функционируют. Мы уберем соответствующую задачу из списка gTasks
    startedAt = DateTime().currentDateTime;
    thread->start();
}

