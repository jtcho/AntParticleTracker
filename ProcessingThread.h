#ifndef PROCESSINGTHREAD_H
#define PROCESSINGTHREAD_H

#include <QThread>
#include <QTGui>
#include "opencv/highgui.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>

#include "Subject.h"
#include "ImageHandler.h"
#include "ImageData.h"
#include "Structures.h"
#include "SubjectGroup.h"

using namespace cv;
using namespace std;

class ProcessingThread : public QThread
{
    Q_OBJECT
public:
    ProcessingThread(ImageHandler* iHandler, ImageData* iData);
    void loadFrame();
    void dropFrame();
    void stopProcessingThread();

    //Group Handling
    Point3_<uchar> setGroupColor(QPoint pos, int ID);
    void removeGroup(int ID);
    SubjectGroup* getSubjectGroup(Point3_<uchar> color);
    SubjectGroup* getSubjectGroup(int ID);

    void addSubject(int groupID, int subjectID, QRectF bound, Mat source, int frameIndex);
    Subject* getSubject(int subjectID, int groupID);

    //GETTERS
    Mat getCurrentFrame();
    int getCurrentFrameIndex();
public slots:
    void updateSubjects();
private:
    volatile bool stopped;
    bool hasBackgroundPalette;

    Mat currentFrame;
    int currentIndex;
    StringGroupMap groups;
    IntGroupMap int_groups;
    IntClusterMap int_currentForeground;
    Mat backgroundPalette;

    QMutex stoppedMutex;
    QMutex frameProtectMutex;
    QMutex groupsMutex;
    ImageHandler* imageHandler;
    ImageData* imageData;

    //Handles the data processing, called from RUN
    void process();
    //Performs Weighted K-Means Algorithm
    int awkmeans(Mat image, QRectF bound, list<Point3_<uchar> > *centers, IntClusterMap *clusters, ColorFrequencyMap *colorFreqs, int groupID);
    //Shrinks Bounding Box to Color represented by Cluster M
    QRectF fitRect(Mat dest, QRectF bound, IntClusterMap* clusters, int m);
    QRectF fitBinRect(Mat image);
    //Returns the Center of Mass of the Blob contained in the Matrix.
    Point2f getCenterOfMass(Mat binMat);
    //Returns the Direction of the Blob Contained in the Matrix
    float getDirection(Point2f center, Mat binMat);
    //Extracts a bitmap containing only 2 colors, background and foreground (as specified by clusterID).
    Mat extractBinaryMat(Mat image, QRectF frame, IntClusterMap* clusters, int clusterID);
    Mat extractBinaryMat(Mat image, QRectF frame, set<std::string> cluster);
    //Extract the number of labels and a matrix of labels corresponding to an image.
    pair<Mat, int> extractComponentLabels(Mat image);
    //Filter out blobs in an image by size. Currently takes the largest blob (but this can be erroneous).
    Mat sizeFilter(Mat image);
    //Removes 1-2 pixel long bridges from the image. A bruteforce method of removing noise.
    Mat removeBridges(Mat image);
    //Masks out an image using a bitmap.
    Mat mask(Mat image, Mat mask);
    //Returns a matrix representing a set of unique background colors.
    Mat findBackgroundColors();
    //Display the palette as colored squares in a window.
    void showPalette(list<Point3_<uchar> > palette, char* windowName, int squareSize);
    //Remove similar colors from the palette.
    void trimBackgroundPalette(list<Point3_<uchar> >* bgPalette);
protected:
    void run();
};

#endif // PROCESSINGTHREAD_H
