#include "ImageHandler.h"
#include <QDebug>

ImageHandler::ImageHandler() {
    //This guarantees that input comes first... despite thread action
    readSlot = new QSemaphore(1);
    procSlot = new QSemaphore(0);

    frameIndex = -1;
}

/*
 * Note: As the frame is being passed by value, don't modify it here.
 */
void ImageHandler::setFrame(const Mat& frame, int newIndex) {
    qDebug() << "Image Handler: Received call to set Frame.";
    currentFrameProtect.lock();
    if (frameIndex != newIndex) {
            currentFrame = frame;
            frameIndex = newIndex;
    } //Note: setFrame should only be called from the CaptureThread
    currentFrameProtect.unlock();
    qDebug() << "Image Handler: Finished set Frame pass.";
}

Mat ImageHandler::getFrame() {
    qDebug() << "Image Handler: Received call to get Frame.";
    currentFrameProtect.lock();
        Mat tempFrame = currentFrame.clone();
    currentFrameProtect.unlock();
    qDebug() << "Image Handler: Returning Frame";
    return tempFrame;
}

void ImageHandler::clear() {
    currentFrameProtect.lock();
        currentFrame.release();
        frameIndex = -1;
    currentFrameProtect.unlock();
}

int ImageHandler::currentIndex() {
    currentFrameProtect.lock();
    int tempFrameIndex = frameIndex;
    currentFrameProtect.unlock();
    return tempFrameIndex;
}

void ImageHandler::getReadSlot() {
//    qDebug() << "readSlot: " << readSlot->available();
//    qDebug() << "ProcessingResources: " <<procSlot->available() << endl;
    readSlot->acquire();
}

void ImageHandler::getProcSlot() {
//    qDebug() << "readSlot: " << readSlot->available();
//    qDebug() << "ProcessingResources: " <<procSlot->available() << endl;
    procSlot->acquire();
}

void ImageHandler::releaseReadSlot() {
//    qDebug() << "readSlot: " << readSlot->available();
//    qDebug() << "ProcessingResources: " <<procSlot->available() << endl;
    readSlot->release();
}

void ImageHandler::releaseProcSlot() {
//    qDebug() << "readSlot: " << readSlot->available();
//    qDebug() << "ProcessingResources: " <<procSlot->available() << endl;
    procSlot->release();
}
