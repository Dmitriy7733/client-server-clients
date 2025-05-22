QT       += core gui widgets network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = udp_server
TEMPLATE = app

INCLUDEPATH += C:/boost_1_88_0
LIBS += -LC:/boost_1_88_0/lib64-msvc-14.3
LIBS += -lboost_thread-vc143-mt-x64-1_88 -lboost_system-vc143-mt-x64-1_88

#LIBS += -LC:\boost_1_88_0\stage/lib
#LIBS += -llibboost_thread-mgw14-mt-x64-1_88 -llibboost_system-mgw14-mt-x64-1_88
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    udp_server.cpp

HEADERS += \
    mainwindow.h \
    udp_server.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
