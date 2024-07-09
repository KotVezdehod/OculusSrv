#ifndef FILECLEANER_H
#define FILECLEANER_H

#include <QObject>
#include <QTimer>


class FileCleaner : public QObject
{
    Q_OBJECT

    QTimer *mTimer = nullptr;

public:
    FileCleaner(){};

    ~FileCleaner()
    {
        mTimer->stop();
        mTimer->deleteLater();
    }

signals:
    void started();


public slots:
    void process();
    void clean();


};

#endif // FILECLEANER_H
