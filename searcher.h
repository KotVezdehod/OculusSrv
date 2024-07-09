#ifndef SEARCHER_H
#define SEARCHER_H

#include <QObject>
#include <QTimer>

#include "sqlite.h"

class Searcher : public QObject
{
    QTimer *timer;

    Q_OBJECT
public:
    explicit Searcher(QObject *parent = nullptr);

public slots:
    void process();

private slots:
    void search();

signals:
    void started();

};

#endif // SEARCHER_H
