#include "diagnostics.h"
#include <sstream>
#include <filesystem>
#include <mutex>
#include <fstream>

namespace fs = std::filesystem;
extern QString *dataFolder;

extern std::mutex *mMutex;

QString Diagnostics::dateTimeTo1cString(QDateTime dtIn)
{
    return dtIn.toString("yyyy.MM.dd HH:mm:ss");
}

QString Diagnostics::toLogInternalStructure()
{

    std::stringstream ss;
    ss << "Дата: " << intentDate.toStdString()
       << "; status: " << status
       << "; Уровень: " << (status ? level.toStdString() : std::string(DEBUG_LEVELS::ERROR))
       << "; Описание: " << description.toStdString()
       << "; Данные: " << "";

    return QString::fromStdString(ss.str());
}

void Diagnostics::throwLocalDiag()
{
    fs::path logPath(dataFolder->toStdString());
    logPath.append("log.txt");
    std::ofstream ofs;
    std::lock_guard<std::mutex> mute(*mMutex);

    ofs.open(logPath,std::ios::app);
    if (ofs.is_open()){
        ofs << toLogInternalStructure().toStdString() << std::endl;
        ofs.close();
    }

    return;
}
