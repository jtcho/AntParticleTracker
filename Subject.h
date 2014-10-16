#ifndef SUBJECT_H
#define SUBJECT_H

#include <QTGui>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <map>
#include <set>
#include "Structures.h"

using namespace cv;

class Subject {
public:
    Subject(QRectF boundFrame, int newID, int frameIndex);
    Subject(QRectF boundFrame, float newDir, int newID, int frameIndex);
    Subject(QRectF boundFrame, QPointF newPos, float newDir, set<string> newColors, int newID, int groupID, int frameIndex);

    //Getters
    QPointF pos();
    float dir();
    std::list<QPointF> getPastPositions();
    std::list<float> getPastDirections();
    set<std::string> getColors();
    int getStartingFrameIndex();
    int getID();
    int getGroupID();
    QRectF getCurrentBoundingFrame();

    //Setters
    void setPos(QPointF newPos);
    void setDirection(float newDir);
    void setColors(set<std::string> newColors);
    void setCurrentBoundingFrame(QRectF boundFrame);

private:
    QPointF position;
    float direction;
    std::list<QPointF> pastPositions;
    std::list<float> pastDirections;
    set<std::string> colors;
    const int ID;
    const int groupID;
    const int startingFrameIndex;
    QRectF currentBoundingFrame;
};

//Maps the Subject's int ID to the Subject pointer.
typedef std::map<int, Subject*> IntSubjectMap;


#endif // SUBJECT_H
