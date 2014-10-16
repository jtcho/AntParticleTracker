#include "ImageData.h"
#include "QDebug"

ImageData::ImageData()
{
    readSlot = new QSemaphore(1);
    writeSlot = new QSemaphore(0);

    halt = false;
    frameIndex = -1;
}

void ImageData::setData(const Mat &frame, int newIndex, IntGroupMap groups) {
    currentFrameProtect.lock();

    if (frameIndex != newIndex) {
        currentFrame = frame;
        frameIndex = newIndex;
        this->groups = groups;
    }

    currentFrameProtect.unlock();
}

void ImageData::updateGroups(IntGroupMap groups) {
    currentFrameProtect.lock();
    this->groups = groups;
    currentFrameProtect.unlock();
}

Mat ImageData::getFrame() {
    currentFrameProtect.lock();

    Mat tempMat = currentFrame.clone();

    currentFrameProtect.unlock();

    return tempMat;
}

IntGroupMap ImageData::getGroups() {
    groupProtect.lock();
    IntGroupMap tempMap = groups;
    groupProtect.unlock();
    return tempMap;
}

int ImageData::currentIndex() {
    currentFrameProtect.lock();
    int tempIndex = frameIndex;
    currentFrameProtect.unlock();
    return tempIndex;
}

void ImageData::clear() {
    currentFrameProtect.lock();

    currentFrame.release();
    frameIndex = -1;
    //backgroundPalette.clear();

    currentFrameProtect.unlock();
}

void ImageData::stop() {
    haltProtect.lock();
    halt = true;
    qDebug() << "Halting image data.";
    haltProtect.unlock();
}

bool ImageData::halted() {
    haltProtect.lock();
    bool stopped = halt;
    haltProtect.unlock();
    return stopped;
}

void ImageData::getReadSlot() {
    readSlot->acquire();
}

void ImageData::releaseReadSlot() {
    readSlot->release();
}

void ImageData::getWriteSlot() {
    writeSlot->acquire();
}

void ImageData::releaseWriteSlot() {
    writeSlot->release();
}
