#ifndef TASKWATCHER_H
#define TASKWATCHER_H

#include <QObject>
#include <QThread>
#include <QTimer>

class TaskWatcher : public QObject
{

    Q_OBJECT
public:
    explicit TaskWatcher(QObject *parent = nullptr);

private slots:
    void process();
    void watch();
    void objectDestroyed(QObject *obj);
    void localServerStarted();
    void searcherStarted();
    void fileCleanerStarted();

private:
    QTimer* timer;
    bool localServerListening = false;
    bool searcherWorking = false;

};

class Task
{

public:
    explicit Task(QObject* objectIn, QString descrIn):
        object(objectIn), description(descrIn){};

    QThread *thread = nullptr;
    QObject *object = nullptr;
    QString description;
    bool deleterBeenCalled = false;
    void start();
    unsigned long long startedAt;
};



#endif // TASKWATCHER_H
