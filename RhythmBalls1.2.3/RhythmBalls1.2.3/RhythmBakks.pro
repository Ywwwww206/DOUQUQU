QT       += core gui widgets multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    circlecropdialog.cpp \
    coverwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    audioprocessor.cpp \
    ball.cpp \
    collisionsound.cpp

HEADERS += \
    circlecropdialog.h \
    coverwidget.h \
    mainwindow.h \
    audioprocessor.h \
    ball.h \
    collisionsound.h

# Default rules
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES +=