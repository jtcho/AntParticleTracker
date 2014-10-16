#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <QMutex>
#include <QSemaphore>
#include <opencv/highgui.h>

using namespace cv;

/*
 * The ImageHandler resides in the CaptureThread.
 * As it is accessed by both the CaptureThread and the ProcessingThread,
 * safety-nets must be installed via the Mutex.
 */
class ImageHandler
{
public:
    ImageHandler();
    void setFrame(const Mat& frame, int newIndex);
    Mat getFrame(); //Returns the current Frame.
    void clear(); //Removes the currentFrame and resets frameIndex.
    int currentIndex(); //Returns the index of the current Frame.

    void getReadSlot();
    void releaseReadSlot();
    void getProcSlot();
    void releaseProcSlot();

private:
    QMutex currentFrameProtect; //Protects the current frame from simultaneous accesses.
    QSemaphore* readSlot;
    QSemaphore* procSlot;

    Mat currentFrame;
    int frameIndex; //Updates constantly.
};

#endif // IMAGEHANDLER_H
