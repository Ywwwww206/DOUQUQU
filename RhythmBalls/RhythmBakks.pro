QT       += core gui widgets multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    audioprocessor.cpp \
    ball.cpp \
    collisionsound.cpp

HEADERS += \
    mainwindow.h \
    audioprocessor.h \
    ball.h \
    collisionsound.h

# Default rules
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target