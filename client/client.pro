#client.pro
QT       += core gui widgets network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = client
TEMPLATE = app

#INCLUDEPATH += C:/boost   #C:\boost_1_88_0

INCLUDEPATH += C:/pa_stable_v190700_20210406/portaudio/include
LIBS += -LC:\pa_stable_v190700_20210406\portaudio\build\msvc\x64\Debug -lportaudio_x64

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    #audiobuffer.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    #audiobuffer.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

