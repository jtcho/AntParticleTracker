#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include "opencv/highgui.h"
#include <QMutex>
#include <QSemaphore>
#include "SubjectGroup.h"
#include "MedianCut.h"

using namespace cv;
using namespace std;

//Used as a buffer between the ProcessingThread and the DisplayThread.
class ImageData
{
public:
    ImageData();

    void setData(const Mat& frame, int newIndex, IntGroupMap groups);
    void updateGroups(IntGroupMap groups);
    Mat getFrame(); //Returns the current Frame.
    IntGroupMap getGroups();
    int currentIndex();
    void clear();
    bool halted();
    void stop();

    void getReadSlot();
    void releaseReadSlot();
    void getWriteSlot();
    void releaseWriteSlot();
private:
    //Protects access to the current frame.
    QMutex currentFrameProtect;
    //Protects access to the background palette.
    QMutex groupProtect;
    //
    QMutex haltProtect;
    //Regulates the reading of the input data from the ProcessingThread.
    QSemaphore* readSlot;
    //Regulates the writing of the data to the DisplayThread.
    QSemaphore* writeSlot;
    //Current Frame Reference, not meant to be modified within here.
    Mat currentFrame;
    int frameIndex; //Updates constantly.
    volatile bool halt;
    IntGroupMap groups;
};

#endif // IMAGEDATA_H
