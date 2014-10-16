#ifndef DISPLAYTHREAD_H
#define DISPLAYTHREAD_H

#include <QThread>
#include <QTGui>
#include "opencv/highgui.h"

#include "Structures.h"
#include "VideoFrame.h"
#include "ImageData.h"
#include "MatToQImage.h"

using namespace cv;

class DisplayThread : public QThread
{
    Q_OBJECT
public:
    DisplayThread(ImageData* imageData);
    //Called upon dropping the video
    void dropFrame();
    void stopDisplayThread();

    //Setters
    void setMouseCursor(int);

    //Geters
    QPoint getMouseCursorPos();
    int getMouseCursorType();
    int getCurrentFrameIndex();
    Mat getCurrentSourceFrame();
signals:
    //Called to let the MainWindow know to repaint.
    void frameUpdate(const QImage &frame, const int);
    void selectionBoxFormed(MouseData);
    void colorPicked(QPoint);
public slots:
    void onMouseMove(QMouseEvent *ev);
    void onMousePress(QMouseEvent *ev);
    void onMouseRelease(QMouseEvent *ev);

    void releaseSelectionBox();

    void showSubjectBoxes(const int);
private:
    //Running status of the thread
    bool stopped;
    //Prevents modifications made to the stopped boolean.
    QMutex stoppedMutex;
    //Prevents modifications made to the stored image (QImage)
    QMutex imageProtectMutex;
    //Prevents modifications made to the stored frame (Mat)
    QMutex frameProtectMutex;
    //Prevents modifications made to to stored mouse data.
    QMutex mouseProtectMutex;
    //Prevents modifications made to flags.
    QMutex flagProtectMutex;
    //
    QMutex paintProtectMutex;
    //Stored frame from ProcessingThread
    Mat currentFrame;
    //Current frame index
    int currentIndex;
    //Converted from currentFrame
    QImage sourceImage;
    //
    QImage modImage;
    //Stores a reference to ImageData
    ImageData *imageData;
    //Contains Subject Groups, mapped by GroupID.
    IntGroupMap groups;
    //Constructs the QImage from the modified Matrix and calls frameUpdate();
    MouseData mouseData;
    //Used to calculate the selection box.
    QPoint startPoint;
    //Determines whether or not to draw a selection box.
    bool drawBox;
    //Holds reference to the selectionBox.
    QRect* selectionBox;
    //Determines whether or not to draw the subject boxes, -1 if no, >0 if yes, corresponding to group ID.
    int subjectBox;
    //
    VideoFrame* videoFrame;
    //Handles Painting performed on the QImage.
    void paint();
protected:
    void run();
};

#endif // DISPLAYTHREAD_H
