#include "Subject.h"

Subject::Subject(QRectF boundFrame, int newID, int frameIndex) :
    ID(newID), groupID(-1), startingFrameIndex(frameIndex), currentBoundingFrame(boundFrame)
{
    this->position = boundFrame.center();
    this->direction = 0;
}

Subject::Subject(QRectF boundFrame, float newDir, int newID, int frameIndex) :
    direction(newDir), ID(newID), groupID(-1), startingFrameIndex(frameIndex), currentBoundingFrame(boundFrame)
{
    this->position = currentBoundingFrame.center();
}

Subject::Subject(QRectF boundFrame, QPointF newPos, float newDir, set<string> newColors, int newID, int groupID, int frameIndex) :
    position(newPos), direction(newDir), colors(newColors), ID(newID), groupID(groupID),
    startingFrameIndex(frameIndex), currentBoundingFrame(boundFrame)
{
}

QPointF Subject::pos() {
    return position;
}

float Subject::dir() {
    return direction;
}

std::list<QPointF> Subject::getPastPositions() {
    return pastPositions;
}

std::list<float> Subject::getPastDirections() {
    return pastDirections;
}

set<std::string> Subject::getColors() {
    return colors;
}

int Subject::getStartingFrameIndex() {
    return startingFrameIndex;
}

int Subject::getID() {
    return ID;
}

int Subject::getGroupID() {
    qDebug() << "Group ID Called: " << groupID;
    return groupID;
}

QRectF Subject::getCurrentBoundingFrame() {
    return currentBoundingFrame;
}

void Subject::setCurrentBoundingFrame(QRectF boundFrame) {
    this->currentBoundingFrame = boundFrame;
}

void Subject::setPos(QPointF newPos) {
    pastPositions.push_front(position);
    this->position = newPos;
}

void Subject::setDirection(float newDir){
    pastDirections.push_front(direction);
    this->direction = newDir;
}

void Subject::setColors(set<string> newColors) {
    this->colors = newColors;
}
