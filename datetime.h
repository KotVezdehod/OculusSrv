#ifndef DATETIME_H
#define DATETIME_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimeZone>
#include <QDate>
#include <QTime>

class DateTime
{

public:
    unsigned long currentDate = 0;
    unsigned long currentTime = 0;
    unsigned long long currentDateTime = 0;
    QDateTime mDt;

    DateTime()
    {
        mDt = QDateTime::currentDateTime();
        currentDate = dateTimeToUlongLongDate(mDt);
        currentTime = dateTimeToUlongLongTime(mDt);;
        currentDateTime = dateTimeToUlongLongDateTime(mDt);
    };

    DateTime(QDateTime dtIn) : mDt(dtIn)
    {
        currentDate = dateTimeToUlongLongDate(mDt);
        currentTime = dateTimeToUlongLongTime(mDt);;
        currentDateTime = dateTimeToUlongLongDateTime(mDt);
    };

    DateTime(unsigned long long dtIn)
    {
        mDt = ulongLongToDateTime(dtIn);
        //qDebug()<<mDt;
        currentDate = dateTimeToUlongLongDate(mDt);
        currentTime = dateTimeToUlongLongTime(mDt);;
        currentDateTime = dateTimeToUlongLongDateTime(mDt);
    };

    unsigned long long dateTimeToUlongLongDate(QDateTime dtIn);
    unsigned long long dateTimeToUlongLongTime(QDateTime dtIn);
    unsigned long long dateTimeToUlongLongDateTime(QDateTime dtIn);

    QDateTime ulongLongToDateTime(unsigned long long dtIn);

    unsigned long long operator - (const DateTime dataIn);

    ~DateTime(){};

};

#endif // DATETIME_H
