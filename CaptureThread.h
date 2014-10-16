#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QThread>
#include <QTGui>

#include "opencv/highgui.h"
#include "Structures.h"
#include "ImageHandler.h"

using namespace cv;

class CaptureThread : public QThread {
    Q_OBJECT

public:
    //Constructor, takes in a reference to the imageHandler.
    CaptureThread(ImageHandler* imageHandler);
    //Loads the video specified by the file path. Instantiates VideoCapture.
    bool loadVideo(QString fileName);
    //Releases the video input.
    void dropVideo();
    //Halts Capture Thread.
    void stopCaptureThread();

    Mat getCurrentFrame();
    int getCurrentFrameIndex();

    int getInputSourceWidth();
    int getInputSourceHeight();

    bool isPlaying();
public slots:
    void togglePlayState();
    void play();
    void pause();
    void stepForward();
    void stepBackward();
signals:
    void steppedBack();
    void steppedForward();

    void playStateChanged(const int);

    void playingInit();
    void paused();
    void atStartOfVideo();
    void reachedEndOfVideo();
private:
    CvCapture* cap;
    ImageHandler* imageHandler;
    QMutex stoppedMutex;
    QMutex playingMutex;

    volatile bool playing;
    volatile bool stopped;

    void updateFrame(int);
protected:
    void run();
};

#endif // CAPTURETHREAD_H
