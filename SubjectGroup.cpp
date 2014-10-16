#include "SubjectGroup.h"
#include "Utilities.h"

SubjectGroup::SubjectGroup() : ID(0) {

}

SubjectGroup::SubjectGroup(Point3_<uchar> newColorPoint, int newID) : ID(newID)
{
    this->colorPoint = newColorPoint;
    this->colorHex = color2Hex(newColorPoint);
}

Point3_<uchar> SubjectGroup::getColorPoint() {
    //colorMutex.lock();
    Point3_<uchar> tempCol = colorPoint;
    //colorMutex.unlock();
    return tempCol;
}

std::string SubjectGroup::getColorString() {
    //colorMutex.lock();
    std::string tempCol = colorHex;
    //colorMutex.unlock();
    return tempCol;
}

IntSubjectMap SubjectGroup::getSubjects() {
    //subjectsMutex.lock();
    IntSubjectMap tempMap = subjects;
    //subjectsMutex.unlock();
    return tempMap;
}

int SubjectGroup::getID() {
   return this->ID;
}

void SubjectGroup::setColor(Point3_<uchar> color) {
    //colorMutex.lock();
    this->colorPoint = color;
    this->colorHex = color2Hex(color);
    //colorMutex.unlock();
}

void SubjectGroup::addSubject(Subject *subject) {
    //if (subjects.count(subject->getID())) qDebug() << "Subject of ID exists: " << subject->getID();
    //else qDebug() << "Adding new Subject of ID: " << subject->getID();
    this->subjects[subject->getID()] = subject;
}

void SubjectGroup::removeSubject(int subjectID) {
    subjects.erase(subjectID);
}
