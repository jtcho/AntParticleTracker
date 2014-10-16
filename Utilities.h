#ifndef UTILITIES_H
#define UTILITIES_H

#include "opencv/highgui.h"
#include <QTGui>
#define PI 3.141592625

using namespace cv;

double colorDistance(Point3_<uchar> pt1, Point3_<uchar> pt2);
Point3_<uchar> hex2Color(std::string col);
std::string color2Hex(Point3_<uchar> pixelData);
float rad2Deg(float rads);
float deg2Rad(float degs);

#endif // UTILITIES_H
