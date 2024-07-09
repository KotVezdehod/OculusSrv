#include "httpserver.h"
#include <mutex>
#include <QHttpServer>
#include <QEventLoop>
#include <QtConcurrent>
#include <regex>
#include <QDir>
#include <filesystem>
#include <fstream>
#include "sqlite.h"

namespace fs = std::filesystem;
typedef QList<QPair<QByteArray, QByteArray>> T_Headers;

extern std::mutex *gFileWritingMutex;
extern QString *dataFolder;

HttpServer::HttpServer(QObject *parent)
    : QObject{parent}
{

}

void HttpServer::process()
{
    QHttpServer httpserver;     //Объект сервера должен быть здесь, т.к. если его объявить, как член класса, то он будет в другом потоке...

    //============= loopback для проверки жизни Qt хттп-сервера
    httpserver.route(HTTP_ENTRYES::GET_SERVICE_STATUS, [](const QHttpServerRequest &recv)
    {
        return QtConcurrent::run([&recv]()
        {

            if (recv.method() != QHttpServerRequest::Method::Get){
                QString t("Method not allowed!");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::MethodNotAllowed);

            }


            Sqlite sql;
            Diagnostics diagRes = sql.execStatement("SELECT COUNT(1) AS RowsCount"
                                                 " FROM " + sql.TABLE_NAME);

            if (!diagRes.status){
                QString t("Database operation error!");
                Diagnostics(false, t).throwLocalDiag();
                diagRes.throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            };

            QJsonDocument docOut = QJsonDocument::fromJson(diagRes.obj.toUtf8());
            return QHttpServerResponse(QJsonDocument(docOut[0].toObject()).toJson(), QHttpServerResponse::StatusCode::Ok);
        });
    });

    httpserver.route(HTTP_ENTRYES::REMOVE_FILE, [](const QHttpServerRequest &recv)
    {
        return QtConcurrent::run([&recv]()
        {

            if (recv.method() != QHttpServerRequest::Method::Delete){
                QString t("Method not allowed!");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::MethodNotAllowed);

            }

            QString idLoc = recv.url().path().replace("/removeFile/","");

            Sqlite sql;
            Diagnostics diagRes = sql.execStatement("DELETE"
                                                 " FROM " + sql.TABLE_NAME +
                                                 " WHERE " + QString(DB_FIELDS::FILENAME) + " = " + idLoc);

            if (!diagRes.status){
                QString t("Database operation error!");
                Diagnostics(false, t).throwLocalDiag();
                diagRes.throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            };

            return QHttpServerResponse("", QHttpServerResponse::StatusCode::Ok);
        });
    });

    httpserver.route(HTTP_ENTRYES::GET_FILE_STATUS, [](const QHttpServerRequest &recv)
    {
        return QtConcurrent::run([&recv]()
        {

            if (recv.method() != QHttpServerRequest::Method::Get){
                QString t("Method not allowed!");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::MethodNotAllowed);

            }

            QString idLoc = recv.url().path().replace("/getFileStatus/","");

            Sqlite sql;
            Diagnostics diagRes = sql.execStatement("SELECT status "
                                                 "FROM " + sql.TABLE_NAME +
                                                 " WHERE " + QString(DB_FIELDS::ID) + " = " + idLoc);

            if (!diagRes.status){
                QString t("Database operation error!");
                Diagnostics(false, t).throwLocalDiag();
                diagRes.throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            };

            return QHttpServerResponse(diagRes.obj, QHttpServerResponse::StatusCode::Ok);
        });
    });

    httpserver.route(HTTP_ENTRYES::NEW_FILE, [](const QHttpServerRequest &recv)
    {
        return QtConcurrent::run([&recv]()
        {

            if (recv.method() != QHttpServerRequest::Method::Post){
                QString t("Method not allowed!");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::MethodNotAllowed);
            }

            T_Headers headers = recv.headers();

            QString fileExtension;
            QString patternToFind;

            for (T_Headers::ConstIterator header = headers.constBegin(); header != headers.constEnd(); std::advance(header,1)){
                if (QString::fromUtf8(header->first).compare("ext",Qt::CaseSensitivity::CaseInsensitive)==0){
                    fileExtension = header->second;
                }else if(QString::fromUtf8(header->first).compare("pattern",Qt::CaseSensitivity::CaseInsensitive)==0){
                    patternToFind = QByteArray::fromBase64(header->second);
                }
            };

            if (fileExtension.isEmpty()){
                QString t("File extension is not defined!");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::BadRequest);
            };

            if (patternToFind.isEmpty()){
                QString t("Pattern to find is not defined!");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::BadRequest);
            };


            QString fileNameWOExt = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);
            QString fName(fileNameWOExt);
            fName+=".BinData";

            fs::path filePath(dataFolder->toStdString());
            filePath.append(fName.toStdString());

            std::ofstream ofs(filePath, std::ios::trunc);

            if (!ofs.is_open()){
                QString t("Can't create local image file.");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            };

            ofs.close();
            ofs.open(filePath, std::ios::binary);
            if (!ofs.is_open()){
                QString t("Can't create local image file.");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            };

            int sz = recv.body().size();

            ofs.write(recv.body(), sz);
            ofs.close();

            Sqlite sql;
            DbStructure row;
            row.setValue(DB_FIELDS::FILE, QString::fromStdString(filePath.string()));
            row.setValue(DB_FIELDS::STATUS, 3);
            row.setValue(DB_FIELDS::PATTERN, patternToFind);
            row.setValue(DB_FIELDS::FILENAME, fileNameWOExt);
            Diagnostics diagRes = sql.insertRow(&row);

            if (!diagRes.status){
                fs::remove(filePath);
                QString t("Database operation error.");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            };

            DataRowsSet drs = sql.getRowsByOneParameter(DB_FIELDS::FILENAME, fileNameWOExt);

            if (drs.empty()){
                fs::remove(filePath);
                QString t("Database operation error (can't obtain row id by filename from database).");
                Diagnostics(false, t).throwLocalDiag();
                return QHttpServerResponse(t, QHttpServerResponse::StatusCode::InternalServerError);
            }

            int idLoc = drs[0].value(DB_FIELDS::ID).intValue;


            return QHttpServerResponse(QJsonDocument(QJsonObject{{DB_FIELDS::ID,idLoc},{DB_FIELDS::FILENAME,fileNameWOExt}}).toJson(),QHttpServerResponse::StatusCode::Ok);
        });
    });

    httpserver.listen(QHostAddress::Any, 2612);
    emit started();
    QEventLoop evLoop;
    evLoop.exec();

    QThread::currentThread()->exit();
}
