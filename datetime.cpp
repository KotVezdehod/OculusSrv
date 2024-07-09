#include "datetime.h"

//====== DateTime
unsigned long long DateTime::dateTimeToUlongLongDate(QDateTime dtIn)
{
    return dtIn.date().year()*10000 + dtIn.date().month()*100 + dtIn.date().day();
}

unsigned long long DateTime::dateTimeToUlongLongTime(QDateTime dtIn)
{
    return dtIn.time().hour()*10000 + dtIn.time().minute()*100 + dtIn.time().second();
}

unsigned long long DateTime::dateTimeToUlongLongDateTime(QDateTime dateTimeIn)
{
    return dateTimeIn.date().year()*pow(10,10) + dateTimeIn.date().month()*pow(10,8) + dateTimeIn.date().day()*pow(10,6) +
            dateTimeIn.time().hour()*pow(10,4) + dateTimeIn.time().minute()*pow(10,2) + dateTimeIn.time().second();
}

QDateTime DateTime::ulongLongToDateTime(unsigned long long dtIn)
{

    unsigned long long yearLoc = dtIn/10000000000;
    unsigned long long monthLoc = dtIn/100000000-yearLoc*100;
    unsigned long long dayLoc = dtIn/1000000 - yearLoc*10000 - monthLoc*100;

    unsigned long long hoursLoc = 0;
    hoursLoc = dtIn/10000-yearLoc*1000000-monthLoc*10000-dayLoc*100;

    unsigned long long minsLoc = 0;
    minsLoc = dtIn/100 - yearLoc*100000000 - monthLoc*1000000 - dayLoc*10000 - hoursLoc*100;

    unsigned long long secsLoc = 0;
    secsLoc = dtIn - yearLoc*10000000000 - monthLoc*100000000 - dayLoc*1000000 - hoursLoc*10000 - minsLoc*100;

    return QDateTime(QDate(yearLoc, monthLoc, dayLoc), QTime(hoursLoc, minsLoc, secsLoc));
}

unsigned long long DateTime::operator - (const DateTime dataIn)
{
    QDateTime left = this->mDt;
    QDateTime right = dataIn.mDt;
    return left.toMSecsSinceEpoch() - right.toMSecsSinceEpoch();
}
