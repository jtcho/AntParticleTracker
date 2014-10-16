#ifndef STRUCTURES_H
#define STRUCTURES_H

#pragma once
#include <QTGui>
#include "MedianCut.h"
#include <set>

using namespace std;
using namespace cv;

//Mapping a numerical ID to a cluster (list of points)
typedef map<int, set<std::string> > IntClusterMap;

//Mapping a string color representation to a cluster.
typedef map<std::string, set<std::string> > StringClusterMap;
//Mapping a pixel color to its frequency.
typedef map<std::string, float> ColorFrequencyMap;

// MouseData structure definition
struct MouseData{
    QPoint pos;
    QRect selectionBox;
    bool leftButtonRelease;
    bool rightButtonRelease;
    int cursorType;
};

const float DIST_THRESH_RGB = 30;

const float DIST_THRESH_LAB = 30;

const int DIR_SEARCH_THRESH = 5;

//Defines enumeration of Cursor Types.
enum CURSOR_TYPES {
    DEFAULT = 0,
    COLOR_PICKER = 1,
    SUBJECT_SELECTOR = 2,
    REGION_SELECTOR = 3
};

//Defines enumeration of Play States.
enum PLAY_STATES {
    PAUSE = 0,
    PLAY = 1,
    START_OF_VIDEO = 2,
    END_OF_VIDEO = 3
};

#endif // STRUCTURES_H
