#ifndef SUBJECT_GROUP_H
#define SUBJECT_GROUP_H

#include <QTGui>
#include <QMutex>
#include <opencv/highgui.h>
#include "Subject.h"

class SubjectGroup {
public:
    SubjectGroup();
    SubjectGroup(Point3_<uchar> newColorPoint, int ID);

    //GETTERS
    Point3_<uchar> getColorPoint();
    std::string getColorString();
    IntSubjectMap getSubjects();
    int getID();

    //SETTERS
    void setColor(Point3_<uchar> color);
    void addSubject(Subject* subject);
    void removeSubject(int subjectID);

private:
    //Color is guaranteed to be a unique center m in LAB
    Point3_<uchar> colorPoint;
    std::string colorHex;
    //Contains a list of all subjects guaranteed to be of the same color cluster.
    IntSubjectMap subjects;
    const int ID;
    //Thread Protection
    //QMutex colorMutex;
    //QMutex subjectsMutex;
};

//Mapping string color representation to a Group.
typedef std::map<std::string, SubjectGroup*> StringGroupMap;
//Mapping of a numerical ID to a Group.
typedef std::map<int, SubjectGroup*> IntGroupMap;

#endif // SUBJECT_GROUP_H
