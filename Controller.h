#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "CaptureThread.h"
#include "ProcessingThread.h"
#include "DisplayThread.h"
#include "Structures.h"

#include <QTGui>
#include <opencv/highgui.h>

class Controller : public QObject {
    Q_OBJECT

public:
    Controller();
    ~Controller();

    /*
     * The CaptureThread takes in input and passes the frame to the ImageHandler.
     */
    CaptureThread *captureThread;
    /*
     * The ProcessingThread takes in the input frame from the ImageHandler.
     * It literally processes the frame's pixel data and adds to the data sets.
     * The ProcessingThread can only hold onto one frame at a time. It maintains a current frameIndex.
     * Now that I think about it, I should have two modes, MANUAL (DEBUG) and AUTOMATIC.
     * MANUAL : Step-by-step forward/backward, manually adjusting tracker information as necessary...
     *          Note that moving backwards will cause the program to wipe data for later frames.
     * AUTO   : Processing occurs on its own, frame by frame. May be prone to error, but is automatic.
     * The ProcessingThread stores all of the recorded data, which may be accessed but not modified publicly.
     * The ProcessingThread then passes on its current Frame and FrameIndex to the DisplayThread.
     */
    ProcessingThread *processThread;
    /*
     * The DisplayThread takes in the input frame from the ProcessThread.
     * All image data is deep-copied to a new display frame, which will be painted over to show labels (e.g. regions).
     * The finished display frame is then passed to the GUI thread (MainWindow) to be shown.
     */
    DisplayThread *displayThread;

    /*
     * Called by the Main Window upon loading a video file.
     * Instantiates the threads and starts them.
     *
     * Note: Don't worry about having threads waste processing power by
     * running perpetually, even when there's nothing to do.
     * Use of semaphores will cause threads to wait for resources.
     */
    bool loadVideo(QString, int, int, int);

    /*
     * Causes the various threads to stop gracefully.
     * Allow each thread to handle its own resources, and simply delegate calls to them.
     */
    void dropVideo();

    //Halts the capture thread gracefully.
    void stopCaptureThread();
    //Deletes the capture thread... this is dangerous!
    void deleteCaptureThread();
    //Halts the processing thread gracefully.
    void stopProcessingThread();
    //Deletes the processing thread.
    void deleteProcessingThread();
    //Halts the display thread gracefully.
    void stopDisplayThread();
    //Deletes the display thread.
    void deleteDisplayThread();
};


#endif // CONTROLLER_H
