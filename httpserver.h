#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>

class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = nullptr);

    struct HTTP_ENTRYES{
        constexpr static const char NEW_FILE[] = "/newFile";
        constexpr static const char GET_FILE_STATUS[] = "/getFileStatus/(\\d{0,10})";
        constexpr static const char REMOVE_FILE[] = "/removeFile/(\\d{0,10})";
        constexpr static const char GET_SERVICE_STATUS[] = "/getServiceStatus";
    };

public slots:
    void process();

signals:
    void started();

};

#endif // HTTPSERVER_H
