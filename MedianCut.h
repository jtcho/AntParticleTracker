/* Copyright (c) 2012 the authors listed at the following URL, and/or
the authors of referenced articles or incorporated external code:
http://en.literateprograms.org/Median_cut_algorithm_(C_Plus_Plus)?action=history&offset=20080309133934

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Retrieved from: http://en.literateprograms.org/Median_cut_algorithm_(C_Plus_Plus)?oldid=12754
*/

#ifndef MEDIAN_CUT_H_
#define MEDIAN_CUT_H_

#include <list>
#include <opencv/highgui.h>

//Number of Dimensions of the Point.
//In this case 3, because RGB has 3 data-values.
const int NUM_DIMENSIONS = 3;

//To avoid conflict with CV::Point, use the namespace pt.
namespace pt {

class Point
{
public:
    unsigned char x[NUM_DIMENSIONS];
};

/*
 * The Block class represents cube in 3-D space.
 * It is defined and stored as two-opposite corner points.
 */
class Block
{
    Point minCorner, maxCorner;
    int pointsLength;
    Point* points;
public:
    Block(Point* points, int pointsLength);
    Block (cv::Mat* image);

    Point* getPoints();

    void freePoints();

    int numPoints() const;
    int longestSideIndex() const;
    int longestSideLength() const;
    bool operator<(const Block& rhs) const;
    void shrink();
private:
    template <typename T>
    static T min(const T a, const T b)
    {
        if (a < b)
            return a;
        else
            return b;
    }

    template <typename T>
    static T max(const T a, const T b)
    {
        if (a > b)
            return a;
        else
            return b;
    }
};
template <int index>
class CoordinatePointComparator
{
public:
    bool operator()(Point left, Point right)
    {
        return left.x[index] < right.x[index];
    }
};

//Non-Class Member Mehods
std::list<Point> medianCut(cv::Mat* image, unsigned int desiredSize);

}

#endif /* #ifndef MEDIAN_CUT_H_ */
