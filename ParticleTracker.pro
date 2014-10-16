#-------------------------------------------------
#
#
#
#-------------------------------------------------

QT += core gui widgets

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -stdlib=libc++

TARGET = ParticleTracker
TEMPLATE = app

SOURCES +=\
    MatToQImage.cpp \
    CaptureThread.cpp \
    Controller.cpp \
    ImageHandler.cpp \
    VideoFrame.cpp \
    Utilities.cpp \
    SubjectGroup.cpp \
    Subject.cpp \
    ProcessingThread.cpp \
    MedianCut.cpp \
    MainWindow.cpp \
    ImageData.cpp \
    DisplayThread.cpp \
    DisjointSets.cpp \
    Main.cpp

HEADERS  += \
    MatToQImage.h \
    CaptureThread.h \
    Controller.h \
    Structures.h \
    ImageHandler.h \
    MainWindow.h \
    VideoFrame.h \
    Utilities.h \
    SubjectGroup.h \
    Subject.h \
    MedianCut.h \
    ImageData.h \
    DisplayThread.h \
    DisjointSets.h \
    ProcessingThread.h

FORMS += mainwindow.ui

CONFIG += c++11

INCLUDEPATH += /usr/local/include\

LIBS += -L/usr/local/lib \
     -lopencv_core \
     -lopencv_imgproc \
     -lopencv_features2d\
     -lopencv_highgui

RESOURCES += \
    Icons.qrc
