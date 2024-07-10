#include "filecleaner.h"
#include <QTimer>
#include <QEventLoop>
#include <QDebug>
#include <QThread>
#include <QDir>
#include <QDirIterator>
#include "sqlite.h"

extern QString *dataFolder;
extern std::mutex *gFileWritingMutex;

void FileCleaner::process()
{
    mTimer = new QTimer();
    mTimer->setSingleShot(false);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(clean()));
    mTimer->start(30000);

    emit started();
    return;
}

void FileCleaner::clean()
{

    std::lock_guard<std::mutex> mutexLoc(*gFileWritingMutex);

    QDirIterator iter(*dataFolder, {"*.BinData"}, QDir::Files);
    Sqlite sql;

    QString statement;

    while (iter.hasNext())
    {
        statement = "SELECT 1 "
                    "FROM DataPool "
                    "WHERE fileName = 'fname'";

        QString pathFileName(iter.next());
        QFileInfo fLoc(pathFileName);
        QString fName = fLoc.fileName();
        QString suf = fLoc.suffix();
        if (!suf.isEmpty()){
            fName = fName.mid(0, fName.size() - suf.size() - 1);
        }else{
            if (fName.mid(fName.size()-1,1)=="."){
                fName = fName.mid(0, fName.size() - 1);
            }
        }
        statement.replace("fname", fName);



        QVector<QMap<QString,DataValue>> vec = sql.getRowsByStatement(&statement);

        if (vec.size()==0)
        {
            QFile::remove(pathFileName);
            //Diagnostics(true, "FileCleaner::clean: удален файл " + pathFileName + " (ссылка на него отсутствует в БД)").throwLocalDiag();
        }
    }

    QString delStatement;

    statement = "SELECT file, id "
                "FROM DataPool "
                "WHERE file <> ''";

    QVector<QMap<QString,DataValue>> vec1 = sql.getRowsByStatement(&statement);

    for (QVector<QMap<QString,DataValue>>::ConstIterator cIt = vec1.cbegin(); cIt!=vec1.cend(); std::advance(cIt,1))
    {

       delStatement = "DELETE FROM DataPool WHERE id = id1";

        QString filePath(cIt->value("file").strValue);

        if (!QFile::exists(filePath))
        {
            delStatement.replace("id1",QString::number(cIt->value(DB_FIELDS::ID).intValue));
            sql.execStatement(delStatement);
            //Diagnostics(true, "FileCleaner::clean: удалена запись БД, указывающая на отсутствующий файл" + cIt->value("file").strValue).throwLocalDiag();
        }
    }

    return;
}
