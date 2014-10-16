#include <opencv/highgui.h>
#include <QtGui>
#include "Utilities.h"

using namespace std;
using namespace cv;

string itoa(int value, std::string& buf, int base){

    int i = 30;

    buf = "";

    for(; value && i ; --i, value /= base)
        buf = "0123456789abcdef"[value % base] + buf;

    return buf;

}

std::string color2Hex(Point3_<uchar> pixelData) {
    //Converts an 8-bit color of 3 component integers to a hexadecimal string.
    //e.g. Point3_<uchar>(255,255,255) becomes "ffffff"

    //Construct a Hash string (Hexadecimal) from the 3 color components.
    stringstream hexColorStream;
    //Stores temporary hex values before appended to the ostringstream.
    string tempCol;
    //Convert each pixel component integer to hexadecimal and append it to the stream.s
    hexColorStream << itoa(pixelData.x, tempCol, 16);
    hexColorStream << itoa(pixelData.y, tempCol, 16);
    hexColorStream << itoa(pixelData.z, tempCol, 16);

    return hexColorStream.str();
}

Point3_<uchar> hex2Color(string col) {
    //Converts a string representing a hexadecimal concatenation of color component
    //integers to a color vector.
    std::stringstream hexToInt;
    int concatVal;
    Point3_<uchar> tempColor;
    //Converts the entire string to its hexadecimal counterpart as an integer value.
    hexToInt << std::hex << col;
    hexToInt >> concatVal;

    //Separate out the color component values through bitwise operations.
    tempColor.x = (concatVal & 0xff0000) >> 16; //2^4 due to four digit shift
    tempColor.y = (concatVal & 0x00ff00) >> 8; //2^2 due to two digit shift
    tempColor.z = (concatVal & 0x0000ff);

    return tempColor;
}

double colorDistance(Point3_<uchar> pt1, Point3_<uchar> pt2) {
    return sqrt(pow((double)(pt1.x - pt2.x), 2) + pow((double)(pt1.y - pt2.y), 2) + pow((double)(pt1.z - pt2.z), 2));
}

float rad2Deg(float rads) {
    qDebug() << "Converting " << rads << " radians to " << rads*180/PI << " degrees.";
    return rads*180/PI;
}

float deg2Rad(float degs) {
    return degs*PI/180;
}
