#include "Controller.h"

#include <QTGui>
#include <QDebug>

Controller::Controller(){}

Controller::~Controller(){}

/*
 * Called by the Main Window upon loading a video file.
 * Instantiates the threads and starts them.
 *
 * Note: Don't worry about having threads waste processing power by
 * running perpetually, even when there's nothing to do.
 * Use of semaphores will cause threads to wait for resources.
 */
bool Controller::loadVideo(QString filePath, int capThreadPrio,
                           int procThreadPrio, int dispThreadPrio) {
    bool isOpened = false; //local variable
    ImageHandler *imageHandler = new ImageHandler();
    ImageData *imageData = new ImageData();

    captureThread = new CaptureThread(imageHandler);
    processThread = new ProcessingThread(imageHandler, imageData);
    displayThread = new DisplayThread(imageData);

    if ((isOpened = captureThread->loadVideo(filePath))) {
        qDebug() << "Loaded video successfully.";
        qDebug() << "Starting threads...";

        captureThread->start((QThread::Priority)capThreadPrio);
        processThread->start((QThread::Priority)procThreadPrio);

        displayThread->start((QThread::Priority)dispThreadPrio);
    } else {
        qDebug() << "Video failed to open.";
        deleteCaptureThread();
        deleteProcessingThread();
        deleteDisplayThread();
    }
    qDebug() << "Completed loadVideo call from Controller.";
    return isOpened;
}

/*
 * Causes the various threads to stop gracefully.
 * Allow each thread to handle its own resources, and simply delegate calls to them.
 */
void Controller::dropVideo() {
    if (captureThread->isRunning())
        stopCaptureThread();
    captureThread->dropVideo();
    deleteCaptureThread();

    if (displayThread->isRunning())
        stopDisplayThread();
    displayThread->dropFrame();
    deleteDisplayThread();

    if (processThread->isRunning())
        stopProcessingThread();
    processThread->dropFrame();
    //Remember, only the captureThread keeps track of the video.
    deleteProcessingThread();

    qDebug() << "Finished everything!";
}

void Controller::stopCaptureThread() {
    qDebug()<< "About to stop capture thread...";
    captureThread->stopCaptureThread();
    captureThread->wait();
    qDebug() << "Capture thread successfully stopped.";
}

void Controller::deleteCaptureThread() {
    delete captureThread;
    qDebug() << "CaptureThread deleted.";
}

void Controller::stopProcessingThread() {
    qDebug() << "About to stop processing thread...";
    processThread->stopProcessingThread();
    processThread->wait();
    qDebug() << "Processing thread successfully stopped.";
}

void Controller::deleteProcessingThread() {
    delete processThread;
}


void Controller::stopDisplayThread() {
    qDebug() << "About to stop display thread...";
    displayThread->stopDisplayThread();
    displayThread->wait();
    qDebug() << "Display thread successfully stopped.";
}


void Controller::deleteDisplayThread() {
    delete displayThread;
}
