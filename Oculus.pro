QT -= gui
QT += concurrent
QT += httpserver
QT += sql

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        datetime.cpp \
        diagnostics.cpp \
        filecleaner.cpp \
        httpserver.cpp \
        main.cpp \
        searcher.cpp \
        sqlite.cpp \
        taskwatcher.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Oculus/x64/release/ -lOculus
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Oculus/x64/debug/ -lOculus

INCLUDEPATH += $$PWD/../../Oculus
DEPENDPATH += $$PWD/../../Oculus/x64/Release

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../Oculus/x64/release/ -lOculus
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../Oculus/x64/debug/ -lOculus

INCLUDEPATH += $$PWD/../../Oculus
DEPENDPATH += $$PWD/../../Oculus

HEADERS += \
    datetime.h \
    diagnostics.h \
    filecleaner.h \
    httpserver.h \
    searcher.h \
    sqlite.h \
    taskwatcher.h
