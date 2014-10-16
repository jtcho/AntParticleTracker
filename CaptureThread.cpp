#include "CaptureThread.h"

#include <QtDebug>
//config later

//CONSTRUCTOR
CaptureThread::CaptureThread(ImageHandler* imageHandler) :QThread(), imageHandler(imageHandler) {
    playing = false;
    stopped = false;
    this->imageHandler = imageHandler;
}

bool CaptureThread::loadVideo(QString fileName) {
    QByteArray byteArray = fileName.toUtf8();
    const char *p = byteArray.data();

    cap = cvCaptureFromFile(p);
    if (cap) {
        //this->stepForward();
        return true;
    }
    return false;
}

void CaptureThread::dropVideo() {
    qDebug() << "CaptureThread: Dropping Video...";
    cvReleaseCapture(&cap);

    playingMutex.lock();
    playing = false;

    //dropVideo and ProcessingThead::dropFrame are called within the same context
    //that way, dropFrame will only clear its references
    //ImageHandler's clear methods must be called too

    playingMutex.unlock();
    qDebug() << "CaptureThread: Video Dropped.";
}

void CaptureThread::run()
{
    qDebug() << "Capture thread started...";
    qDebug() << cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_COUNT);
    while (1) {
        //STOP CHECK
        stoppedMutex.lock();
        if (stopped) {
            stopped = false;
            stoppedMutex.unlock();
            break;
        } stoppedMutex.unlock();
        //END STOP CHECK

        //qDebug() << "Capture Thread: Locking Playing Mutex.";
        playingMutex.lock();
        if (playing) {
            //qDebug() << "Capture Thread: Passed Playing Check...";
            playingMutex.unlock();
            //Wait until the ImageHandler can read in an image...
            //This method will block until it can.

            qDebug() << "Capture Thread: Waiting for read slot.";
            imageHandler->getReadSlot();
            qDebug() << "Capture Thread: Acquired read slot.";

            updateFrame(imageHandler->currentIndex()+1);

            qDebug() << "Capture Thread: Releasing process slot.";
            imageHandler->releaseProcSlot();
        }
        playingMutex.unlock();
    } qDebug() << "Stopping Capture Thread...";
}

void CaptureThread::updateFrame(int newIndex) {
    qDebug() << "Capture Thread: Updating frame";
    if (newIndex < 0) {
        qDebug() << "Capture Thread: New index received is invalid.";
        pause();
        updateFrame(0);
    }
    else {
        //qDebug() << "Capture Thread: Attempting to change current frame position to: " << newIndex;
        Mat tempFrame;
        cvSetCaptureProperty(cap, CV_CAP_PROP_POS_FRAMES, newIndex);
        //cap->set(CV_CAP_PROP_POS_FRAMES, newIndex);
        //qDebug() << "Capture Thread: Getting Image...";
        IplImage* tempImage = cvQueryFrame(cap);
        qDebug() << "Capture Thread: Got Image!";

        playingMutex.lock();
        if (!playing) {
            if (newIndex == 0) {
                emit playStateChanged(2);
            }
            else emit playStateChanged(0);
        }
        playingMutex.unlock();

        if (!tempImage)
        {
            qDebug() << "At end of video!";
            //at end of video
            pause();
            emit playStateChanged(3);
        }
        else {
            tempFrame = Mat(tempImage);
            imageHandler->setFrame(tempFrame, newIndex);
        }
    }
    qDebug() << "Finished updating frame.";
}

void CaptureThread::togglePlayState() {
    playingMutex.lock();
    playing = !playing;
    if (playing) emit playStateChanged(1);
    else emit playStateChanged(0);
    playingMutex.unlock();
}

void CaptureThread::play() {
    playingMutex.lock();
    playing = true;
    qDebug() << "Capture Thread: Playing initiated.";
    emit playStateChanged(1);
    playingMutex.unlock();
}

void CaptureThread::pause() {
    playingMutex.lock();
    playing = false;
    qDebug() << "Capture Thread: Paused.";
    emit playStateChanged(0);
    playingMutex.unlock();
}


//ATTN: Modify the button enabling on the GUI to be smarter, please.
void CaptureThread::stepBackward(){
    imageHandler->getReadSlot();
    updateFrame(imageHandler->currentIndex()-1);
    imageHandler->releaseProcSlot();
}

void CaptureThread::stepForward() {
    imageHandler->getReadSlot();
    updateFrame(imageHandler->currentIndex()+1);
    imageHandler->releaseProcSlot();
}

bool CaptureThread::isPlaying() {
    playingMutex.lock();
    bool play = playing;
    playingMutex.unlock();
    return play;
}

int CaptureThread::getCurrentFrameIndex() {
    return imageHandler->currentIndex();
}

Mat CaptureThread::getCurrentFrame() {
    return imageHandler->getFrame();
}

int CaptureThread::getInputSourceWidth() {
    return cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH);
}

int CaptureThread::getInputSourceHeight() {
    return cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT);
}

void CaptureThread::stopCaptureThread() {
    stoppedMutex.lock();
        stopped = true;
    stoppedMutex.unlock();
    imageHandler->releaseProcSlot();
}
