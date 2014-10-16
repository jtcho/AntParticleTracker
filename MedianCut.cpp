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

#include <limits>
#include <queue>
#include <algorithm>
#include <QDebug>
#include "MedianCut.h"

using namespace pt;

Block::Block(cv::Mat* image)
{
    //Find the number of pixels within the image.
    const int numPixels = image->rows * image->cols;

    //Using malloc, allocate memory for the points aray.
    this->points = (Point*) malloc(numPixels * sizeof(Point));
    //Don't forget to free(points) later.

    for (int i = 0; i < image->rows; i++) {
        for (int j = 0; j < image->cols; j++) {
            //When flattening a 2D array of m x n size...
            //The array is formed by A[m x n], and what is previously @
            //A[i][j] is found at A[j + i*m].
            Point temp;
            //Get individual RGB values.
            //Be warned! OpenCV stores these pixel color values as BGR..which is backwards!
            for (int k = 0; k < NUM_DIMENSIONS; k++) {
                temp.x[k]= image->data[(image->step * i) + (image->channels() * j)+ k];
            }
            points[j + (i*image->rows)] = temp;
        }
    }
    this->pointsLength = numPixels;
    for(int i=0; i < NUM_DIMENSIONS; i++)
    {
        minCorner.x[i] = std::numeric_limits<unsigned char>::min();
        maxCorner.x[i] = std::numeric_limits<unsigned char>::max();
    }
}

Block::Block(Point* points, int pointsLength)
{
    this->points = points;
    this->pointsLength = pointsLength;
    for(int i=0; i < NUM_DIMENSIONS; i++)
    {
        //Initialize with extrema.
        minCorner.x[i] = std::numeric_limits<unsigned char>::min();
        maxCorner.x[i] = std::numeric_limits<unsigned char>::max();
    }
}

Point* Block::getPoints()
{
    return points;
}

void Block::freePoints() {
    free(points);
}

int Block::numPoints() const
{
    return pointsLength;
}

/*
 * Returns the index (0-2) of the Block's longest side.
 */
int Block::longestSideIndex() const
{
    int m = maxCorner.x[0] - minCorner.x[0];
    int maxIndex = 0;
    for(int i=1; i < NUM_DIMENSIONS; i++)
    {
        int diff = maxCorner.x[i] - minCorner.x[i];
        if (diff > m)
        {
            m = diff;
            maxIndex = i;
        }
    }
    return maxIndex;
}

int Block::longestSideLength() const
{
    int i = longestSideIndex();
    return maxCorner.x[i] - minCorner.x[i];
}

bool Block::operator<(const Block& rhs) const
{
    return this->longestSideLength() < rhs.longestSideLength();
}

/*
 * Shrinks the Block's size down as much as possible while fitting the points within.
 */
void Block::shrink()
{
    int i,j;
    for(j=0; j<NUM_DIMENSIONS; j++)
    {
        minCorner.x[j] = maxCorner.x[j] = points[0].x[j];
    }
    for(i=1; i < pointsLength; i++)
    {
        for(j=0; j<NUM_DIMENSIONS; j++)
        {
            minCorner.x[j] = min(minCorner.x[j], points[i].x[j]);
            maxCorner.x[j] = max(maxCorner.x[j], points[i].x[j]);
        }
    }
}
/*
 * Returns a vector of Points holding the most common pixel values of the image.
 * Uses a divide-and-conquer approach and averages various pixel values.
 */
std::list<Point> pt::medianCut(cv::Mat* image, unsigned int desiredSize)
{
    std::priority_queue<Block> blockQueue;
    //int numPoints = image->rows() * image->cols();

    Block initialBlock(image);
    initialBlock.shrink();
    blockQueue.push(initialBlock);

    //Loop until the desired reduction quota has been met or the block can't be divided
    //evenly anymore.
    while (blockQueue.size() < desiredSize && blockQueue.top().numPoints() > 1)
    {
        Block longestBlock = blockQueue.top();
        blockQueue.pop(); //Remove the top Block, now that it has been stored.

        //Note about pointer arithmetic:
        //Adjusts the pointer reference to the newly calculated memory location.
        Point* begin  = longestBlock.getPoints(); //Retrieve the array of Points from the Block.
        Point* median = longestBlock.getPoints() + (longestBlock.numPoints()+1)/2;
        Point* end    = longestBlock.getPoints() + longestBlock.numPoints();

        switch(longestBlock.longestSideIndex())
        {
            //nth_element sorts the array around a pivot value (median).
            //preceding values are guaranteed to be less than the pivot
            //following values are guaranteed to be greater than the pivot
            //how the other values compare with each other is not determined
            case 0: std::nth_element(begin, median, end, CoordinatePointComparator<0>()); break;
            case 1: std::nth_element(begin, median, end, CoordinatePointComparator<1>()); break;
            case 2: std::nth_element(begin, median, end, CoordinatePointComparator<2>()); break;
        }

        //Split the longestBlock into smaller ones (evenly, however).
        Block block1(begin, median-begin), block2(median, end-median);

        //Shrink the blocks down to fit.
        block1.shrink();
        block2.shrink();

        //Add the blocks the queue. The blocks will be sorted with priority by length.
        blockQueue.push(block1);
        blockQueue.push(block2);
    }

    std::list<Point> result;

    //Iterate through all of the stored Blocks.
    while(!blockQueue.empty())
    {
        Block block = blockQueue.top();
        blockQueue.pop();
        Point* points = block.getPoints();

        int sum[NUM_DIMENSIONS] = {0};
        for(int i=0; i < block.numPoints(); i++)
        {
            for(int j=0; j < NUM_DIMENSIONS; j++)
            {
                sum[j] += points[i].x[j];
            }
        }

        Point averagePoint;
        for(int j=0; j < NUM_DIMENSIONS; j++)
        {
            averagePoint.x[j] = sum[j] / block.numPoints();
        }

        result.push_back(averagePoint);
    }
    initialBlock.freePoints();

    return result;
}

