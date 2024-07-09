#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <QObject>
#include <QString>
#include <QDateTime>

namespace DebugLevels
{
    inline static QString debug(){return "отладка";};
    inline static QString info(){return "информация";};
    inline static QString error(){return "ошибка";};
    inline static QString interface(){return "интерактив";};
    inline static QString system(){return "служебное";};
};

class Diagnostics
{

public:

    //========================= diag levels
    struct DEBUG_LEVELS
    {
        constexpr static const char DEBUG[] = "отладка";
        constexpr static const char INFO[] = "информация";
        constexpr static const char ERROR[] = "ошибка";
        constexpr static const char INTERFACE[] = "интерактив";
        constexpr static const char SYSTEM[] = "служебное";
    };


    Diagnostics(){};

    bool status = true;
    QString description = "";
    QString level = DEBUG_LEVELS::INFO;
    int statusCode = 200;
    QString intentDate = dateTimeTo1cString(QDateTime::currentDateTime());
    QString obj = "";

    QString dateTimeTo1cString(QDateTime dtIn);
    QString toLogInternalStructure();
    void throwLocalDiag();

    Diagnostics(bool statusIn, QString descriptionIn) :
        status(statusIn), description(descriptionIn)
    {
        intentDate = dateTimeTo1cString(QDateTime::currentDateTime());
        if (!statusIn)
        {
            level = DEBUG_LEVELS::ERROR;
            statusCode = 500;
        }
    }

    Diagnostics(bool statusIn, QString descriptionIn, int statusCodeIn) :
        status(statusIn), description(descriptionIn), statusCode(statusCodeIn)
    {
        intentDate = dateTimeTo1cString(QDateTime::currentDateTime());
        if (!statusIn)
            level = DEBUG_LEVELS::ERROR;

    }

    Diagnostics(bool statusIn, QString descriptionIn, int statusCodeIn, QString levelIn) :
        status(statusIn), description(descriptionIn), level(levelIn), statusCode(statusCodeIn)
    {
        intentDate = dateTimeTo1cString(QDateTime::currentDateTime());
        if (!statusIn)
            level = DEBUG_LEVELS::ERROR;
    }


    Diagnostics(bool statusIn, QString descriptionIn, int statusCodeIn, QString levelIn, QString objIn) :
        status(statusIn), description(descriptionIn), level(levelIn), statusCode(statusCodeIn), obj(objIn)
    {
        intentDate = dateTimeTo1cString(QDateTime::currentDateTime());
        if (!statusIn)
            level = DEBUG_LEVELS::ERROR;
    }

    void fill(Diagnostics *diagIn)
    {
        status = diagIn->status;
        description = diagIn->description;
        level = diagIn->level;
        statusCode = diagIn->statusCode;
        intentDate = diagIn->intentDate;
        obj = diagIn->obj;
    };

};



#endif // DIAGNOSTICS_H
