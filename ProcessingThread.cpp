#include "ProcessingThread.h"
#include "Structures.h"
#include "Utilities.h"
#include "MedianCut.h"
#include "DisjointSets.h"

#include "opencv2/imgproc/imgproc.hpp"

ProcessingThread::ProcessingThread(ImageHandler* iHandler, ImageData* iData) : currentIndex(-1)
{
    stopped = false;
    hasBackgroundPalette = false;
    imageHandler = iHandler;
    imageData = iData;

    cvNamedWindow("BinMask");
    cvNamedWindow("Binary");
//    cvNamedWindow("Ellipse");
}

void ProcessingThread::run() {
    qDebug() << "Processing thread started...";

    while (1) {
        //Get frame from ImageHandler if possible, otherwise wait
        qDebug() << "Processing Thread: Waiting for Processing Slot.";
        imageHandler->getProcSlot();
        qDebug() << "Processing Thread: Acquired Processing Slot.";

        //STOP CHECK
        stoppedMutex.lock();

        if (stopped) {
            stopped = false;
            stoppedMutex.unlock();
            break;
        } stoppedMutex.unlock();
        //END STOP CHECK

        frameProtectMutex.lock();
        currentFrame = imageHandler->getFrame();
        currentIndex = imageHandler->currentIndex();
        frameProtectMutex.unlock();

        qDebug() << "Processing Thread: Releasing Input Slot.";
        imageHandler->releaseReadSlot();

        if (!hasBackgroundPalette) {
            backgroundPalette = findBackgroundColors();
            hasBackgroundPalette = true;
        }

        //Initiate Processing for Current Frame
        process();

    } qDebug() << "Stopping Processing Thread...";
}

void ProcessingThread::process() {
    qDebug() << "Processing Thread: Processing...";
    Mat tempFrame;
    frameProtectMutex.lock();
    //Adjust the Saturation of the Received Image
    cvtColor(currentFrame, tempFrame, CV_BGR2HSV);
    for (int i = 0; i < tempFrame.rows; i++) {
        for (int j = 0; j < tempFrame.cols; j++) {
            tempFrame.data[tempFrame.step*i + tempFrame.channels()*j + 1] += 25;
        }
    }
    cvtColor(tempFrame, currentFrame, CV_HSV2BGR);
    cvtColor(currentFrame, tempFrame, CV_BGR2Lab);
    //Iterate through all of the groups and then their subjects.
    map<int, SubjectGroup*>::iterator it1;
    map<int, Subject*>::iterator it2;

    Subject* subject;
    QRectF searchFrame;
    float angle;
    float newAngle;
    QPointF newPos;
    int groupID;
    Mat binMat, labels;
    int numLabels;
    for (it1 = int_groups.begin(); it1 != int_groups.end() && ! imageData->halted(); it1++) {
        //Iterate through Groups.
        IntSubjectMap tempMap = it1->second->getSubjects();
        for (it2 = tempMap.begin(); it2 != tempMap.end(); it2++) {
            //Iterate through each Subject Group.
            //Get Subject Properties
            subject = it2->second;
            searchFrame = subject->getCurrentBoundingFrame();
            groupID = subject->getGroupID();
            angle = subject->dir();
            Point2f center = Point2f(subject->pos().x(), subject->pos().y());

            //Get Major Axis.
            int a = 1.5*max(searchFrame.height(), searchFrame.width());
            int b = min(searchFrame.height(), searchFrame.width());
            qDebug() << "A: " << a << " , " << "B: " << b;
            Point2f a1 = center, a2 = center, b1 = center, b2 = center;

            //Calculate major axis endpoints.
            a1.x = a*cos(angle) - a*sin(angle);
            a1.y = a*sin(angle) + a*cos(angle);
            a2.x = a*sin(angle) - a*cos(angle);
            a2.y = -a*sin(angle) - a*cos(angle);

            qDebug() << "A1: {" << a1.x << "," << a1.y << "}";
            qDebug() << "A2: {" << a2.x << "," << a2.y << "}";

            //Calculate transverse (minor) axis endpoints.
            b1.x = b*cos(angle) - b*sin(angle);
            b1.y = b*sin(angle) + b*cos(angle);
            b2.x = b*sin(angle) - b*cos(angle);
            b2.y = -b*sin(angle) - b*cos(angle);

            qDebug() << "B1: {" << b1.x << "," << b1.y << "}";
            qDebug() << "B2: {" << b2.x << "," << b2.y << "}";

            //Calculate the new bounding rectangle based upon the min-max coordinates of the points.
            Point2f p1, p2;
            p1.x = max(max(a1.x, a2.x), max(b1.x, b2.x)) + center.x;// + searchFrame.center().x();
            p1.y = max(max(a1.y, a2.y), max(b1.y, b2.y)) + center.y;// + searchFrame.center().y();
            p2.x = min(min(a1.x, a2.x), min(b1.x, b2.x)) + center.x;// + searchFrame.center().x();
            p2.y = min(min(a1.y, a2.y), min(b1.y, b2.y)) + center.y;// + searchFrame.center().y();

            qDebug() << "EFrame Properties: " << floor(p2.x) << ", " << floor(p2.y) << ", " << floor(p1.x - p2.x) << ", " << floor(p1.y - p2.y);

            QRectF eFrame(floor(p2.x), floor(p2.y), floor(p1.x - p2.x), floor(p1.y - p2.y));

            //EFrame serves as the bounding rectangle for the elliptical mask.
            //Create the ellipse mask.
            Mat eMask = Mat::zeros(tempFrame.rows, tempFrame.cols, CV_8UC3);
            ellipse(eMask, center, Size(a, b), rad2Deg(angle-PI/2), 0, 360, Scalar(255, 255, 255), -1);
            eMask = mask(tempFrame, eMask);

            Mat binMat = extractBinaryMat(eMask, eFrame, int_currentForeground[groupID]);

            //Get Components.
            pair<Mat, int> labelPair = extractComponentLabels(binMat);
            labels = labelPair.first;
            numLabels = labelPair.second;
            map<int, int> labelSizes;
            for (int i = 0; i < numLabels; i++) {
                labelSizes[i] = 0;
            }
            for (int i = 0; i < labels.rows; i++) {
                for (int j = 0; j < labels.cols; j++) {
                    if (labels.data[i*labels.step + j*labels.channels()])
                        labelSizes[labels.data[i*labels.step + j*labels.channels()]] += 1;
                }
            }
            bool hasOutlier = false;
            map<int, int>::iterator it3;
            float labelsMean = 0;
            float labelsVariance = 0;
            float labelsMaxVariance = 0;
            int maxLabel = 0;
            int sMaxLabel = 0;
            //Calculate the variance of the map sizes.
            //Determine the two largest values AND calculate the variance between them.
            for (it3 = labelSizes.begin(); it3 != labelSizes.end(); it3++) {
                labelsMean += (*it3).second;
                if ((*it3).second > labelSizes[maxLabel]) {
                    sMaxLabel = maxLabel;
                    maxLabel = (*it3).first;
                } else if (maxLabel == sMaxLabel || (*it3).second > labelSizes[sMaxLabel]) {
                    sMaxLabel = (*it3).first;
                }
                qDebug() << (*it3).first << ": " << (*it3).second;
            }
            labelsMean /= labelSizes.size();
            labelsMaxVariance = pow((double)(labelSizes[maxLabel] - labelsMean), 2);
            for (it3 = labelSizes.begin(); it3 != labelSizes.end(); it3++) {
                labelsVariance += pow((*it3).second - labelsMean, 2);
            }
            labelsVariance /= labelSizes.size();

            qDebug() << "Max Label: (" << maxLabel << ", " << labelSizes[maxLabel] << ")";
            qDebug() << "Second Max Label: (" << sMaxLabel << ", " << labelSizes[sMaxLabel] << ")";
            qDebug() << "Labels Mean: " << labelsMean;
            qDebug() << "Labels Variance: " << labelsVariance;
            qDebug() << "Labels Maxes Variance: " << labelsMaxVariance;

            hasOutlier = (labelsMaxVariance > labelsVariance);
            if (! hasOutlier) {
                //K-Means Data Containers
                list<Point3_<uchar> >* centers = new list<Point3_<uchar> >();
                IntClusterMap* clusters = new IntClusterMap();
                ColorFrequencyMap* colorFreqs = new ColorFrequencyMap();
                list<Point3_<uchar> >::iterator it1, it2;

                int kruns = awkmeans(eMask, eFrame, centers, clusters, colorFreqs, groupID);
                qDebug() << "Ran k-means for " << kruns << " runs.";

                double minDistance = std::numeric_limits<double>::max();
                int m = -1, k;
                for (it1 = centers->begin(), k = 0; it1 != centers->end(); it1++, k++) {
                    double distance = colorDistance(int_groups[groupID]->getColorPoint(), *it1);
                    if (distance < minDistance) {
                        minDistance = distance;
                        m = k;
                        it2 = it1;
                    }
                }
                //qDebug() << "Found minimum distance: " << minDistance;
                //qDebug() << "Adding clusters[m] set to group: " << groupID << " for center " << m;

                //Add newly found colors of set to stored cluster. Set properties guarantee uniqueness.
                int_currentForeground[groupID].insert((*clusters)[m].begin(), (*clusters)[m].end());
                //Check for convergence. (Later)

                //Update Binary Matrix.
                binMat = extractBinaryMat(eMask, eFrame, int_currentForeground[groupID]);
            }

            IplImage* tempImg = new IplImage(binMat);
            cvShowImage("Binary", tempImg);

            //TEMPORARY - get largest cluster
            //binMat = sizeFilter(binMat);

            //erode(binMat, binMat, getStructuringElement(cv::MORPH_RECT, Size(1, 1)));
            //dilate(binMat, binMat, getStructuringElement(cv::MORPH_RECT, Size(1, 1)));
            //cvShowImage("BinMask", new IplImage(binMat));

            Point2f tempPos = getCenterOfMass(binMat);
            newAngle = getDirection(Point2f(tempPos.y, tempPos.x), binMat);
            qDebug() << "Angle: " << newAngle;

            //Rectangle has coordinates with respect to binMat.
            //Recall that binMat is fixed within the eFrame.
            QRectF tempRect = fitBinRect(binMat);
            searchFrame.setLeft(tempRect.left() + eFrame.left());
            searchFrame.setTop(tempRect.top() + eFrame.top());
            searchFrame.setWidth(tempRect.width());
            searchFrame.setHeight(tempRect.height());

            subject->setCurrentBoundingFrame(searchFrame);
            subject->setPos(QPointF(tempPos.x + eFrame.left(), tempPos.y + eFrame.top()));
            subject->setDirection(newAngle);
        }
    }

    frameProtectMutex.unlock();
    imageData->getReadSlot();
    frameProtectMutex.lock();
    imageData->setData(currentFrame, currentIndex, int_groups);
    frameProtectMutex.unlock();

    //qDebug() << "Processing Thread: Releasing write slot for imageData.";
    imageData->releaseWriteSlot();
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%% UTILITIES %%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

Mat ProcessingThread::findBackgroundColors() {
    //Called at load-video to do a "rough" analysis of the image.
    //Colors of higher frequency are most likely to be background colors.
    frameProtectMutex.lock();
    list<pt::Point> bgPalettePT = pt::medianCut(&currentFrame, 24);
    frameProtectMutex.unlock();
    list<Point3_<uchar> > bgPalette;
    list<pt::Point>::iterator it1;
    for (it1 = bgPalettePT.begin(); it1 != bgPalettePT.end(); it1++) {
        bgPalette.push_back(Point3_<uchar>(it1->x[0], it1->x[1], it1->x[2]));
    }
    trimBackgroundPalette(&bgPalette);
    //showPalette(bgPalette, "Palette", 40);

    Mat paletteMat(1, bgPalette.size(), CV_8UC3);
    list<Point3_<uchar> >::iterator it2;
    int i;
    for (it2 = bgPalette.begin(), i=0; it2 != bgPalette.end(); it2++, i++) {
        paletteMat.data[paletteMat.channels()*i] = it2->x;
        paletteMat.data[paletteMat.channels()*i + 1] = it2->y;
        paletteMat.data[paletteMat.channels()*i + 2] = it2->z;
    }

    return paletteMat;
}

void ProcessingThread::trimBackgroundPalette(list<Point3_<uchar> >* bgPalette) {
    list<Point3_<uchar> >::iterator it1, it2;
    for (it1 = bgPalette->begin(); it1 != bgPalette->end(); it1++) {
        for (it2 = bgPalette->begin(); it2 != bgPalette->end();) {
            if (it1 != it2) {
                double distance = colorDistance(*it1, *it2);
                if (distance < DIST_THRESH_RGB)
                    it2 = bgPalette->erase(it2);
                else it2++;
            } else it2++;
        }
    }
}

void ProcessingThread::addSubject(int groupID, int subjectID, QRectF bound, Mat source, int frameIndex) {
    Mat dest;
    QRectF fittedBound;
    list<Point3_<uchar> >* centers = new list<Point3_<uchar> >();
    IntClusterMap* clusters = new IntClusterMap();
    ColorFrequencyMap* colorFreqs = new ColorFrequencyMap();
    list<Point3_<uchar> >::iterator it1, it2;

    //Fix negative widths and heights.
    bound = bound.normalized();
    //Convert RGB color space to CIEL*a*b*
    cvtColor(source, dest, CV_BGR2Lab);
    //Perform comprehensive k-means.
    int numKRuns= awkmeans(dest, bound, centers, clusters, colorFreqs, groupID);
    //qDebug() << "Ran k-means for " << numKRuns << " time steps.";
    //Determine which cluster is the desired one by finding the minimum distance between
    //the currently selected group's color and each center.
    double minDistance = std::numeric_limits<double>::max();
    int m, k;
    for (it1 = centers->begin(), k = 0; it1 != centers->end(); it1++, k++) {
        double distance = colorDistance(int_groups[groupID]->getColorPoint(), *it1);
        //qDebug() << "\t" << k << ": " << distance;
        if (distance < minDistance) {
            //Update the minimum to represent this new min.
            minDistance = distance;
            //Update m to represent the new aligned cluster.
            m = k;
            //Update it2 to hold a reference to the new aligned cluster color.
            it2 = it1;
        }
    }
    //Store clusters[m] set in the foreground maps.
    //qDebug() << "Adding clusters[m] set to group: " << groupID;
    int_currentForeground[groupID] = (*clusters)[m];
    //Now m represents the cluster index in "clusters" and "centers" corresponding to the group color.
    //Now to check for the right colors, all we need to do is see if the color exists in the clusters[m] set.
    fittedBound = fitRect(dest, bound, clusters, m);
    if (fittedBound.top() != -1) {
        //fittedBound.adjust(DIR_SEARCH_THRESH * -1, DIR_SEARCH_THRESH * -1, DIR_SEARCH_THRESH, DIR_SEARCH_THRESH);
        Mat binMat = extractBinaryMat(dest, fittedBound, clusters, m);
        binMat = sizeFilter(removeBridges(sizeFilter(binMat)));
        //Calculate the angle formed by the axis of inertia and the x-axis.
        //Note: Due to the nature of inverse trigonometric functions, a value returned by the getDirection function can mean 4 different things.
        //An angle formed with respect to the x-axis will take any value between 0-90, in any quadrant.
        //When processing, use past-states to determine which quadrant/value is the most sensible.
        Point2f cen = getCenterOfMass(binMat);
        float dir = getDirection(Point2f(cen.y, cen.x), binMat);
        //OpenCV Format:: Regular X and Y are swapped.
        //Qt Format:: Reflected across X axis.
        cen.x += fittedBound.top();
        cen.y += fittedBound.left();
        //qDebug() << "Adding Subject with Position: " << cen.x << "," << cen.y;
        //Create a new Subject pointer with the given information.
        Subject* tempSubject = new Subject(fittedBound, QPointF(cen.y, cen.x), dir, (*clusters)[m], subjectID, groupID, frameIndex);
        //Store subject in map.
        int_groups[groupID]->addSubject(tempSubject);

        //Pass update to DisplayThread.
        updateSubjects();
    }
}

Point2f ProcessingThread::getCenterOfMass(Mat binMat) {
    //Find the center of mass, a.k.a. the first moments.
    float x_ = 0, y_ = 0;
    float tempSumX = 0, tempSumY = 0, total = 0;
    int step = binMat.step;
    int channels = binMat.channels();
    for (int i = 0; i < binMat.rows; i++) {
        //When finding the weighted x mean, sum the products of the x-row value and the corresponding b(x,y) values in that row. Then divide by the sum of 1's in the row.
        for (int j = 0; j < binMat.cols; j++) {
            if (binMat.data[step*i + channels*j]) {
                tempSumX++; //Acquire sum of 1's in the row.
                total++;
            }
        }
        //Take the prouct of the x-row value and the sum of 1's, then add to the total.
        x_ += i*tempSumX;
        tempSumX = 0;
    }
    x_ /= total;
    for (int j = 0; j < binMat.cols; j++) {
        for (int i = 0; i < binMat.rows; i++) {
            //Iterate column order wise.
            if (binMat.data[step*i + channels*j]) {
                tempSumY++;
            }
        }
        y_ += j*tempSumY;
        tempSumY = 0;
    }
    y_ /= total;
    Point2f tempPt(y_, x_);
    return tempPt;
}

float ProcessingThread::getDirection(Point2f center, Mat binMat) {
    //Find the angle of the axis of minimum inertia of the cluster to identify the orientation.
    //This is the axis of least 2nd moment.
    //Find the center of mass, a.k.a. the first moments.
    float x_ = center.x, y_ = center.y;
    float tempSumX = 0, tempSumY = 0;
    int step = binMat.step;
    int channels = binMat.channels();
    float a = 0, b = 0, c = 0;
    tempSumX = 0;
    tempSumY = 0;
    for (int i = 0; i < binMat.rows; i++) {
        for (int j = 0; j < binMat.cols; j++) {
            if (binMat.data[step*i + channels*j]) {
                tempSumX++;
                b += (i-x_)*(j-y_);
            }
        }
        a += pow((float)(i-x_), 2)*tempSumX;
        tempSumX = 0;
    }

    b*=2;
    for (int j = 0; j < binMat.cols; j++) {
        for (int i = 0; i < binMat.rows; i++) {
            if (binMat.data[step*i + channels*j]) {
                tempSumY++;
            }
        }
        c += pow((float)(j-y_), 2)*tempSumY;
        tempSumY = 0;
    }
    return acos((a-c)/sqrt(pow(b, 2) + pow(a-c,2)))/2;
}

//Connected Component Labeling to Identify Blobs.
pair<Mat, int> ProcessingThread::extractComponentLabels(Mat image) {
    Mat labels = Mat::zeros(image.rows, image.cols, CV_8UC1);
    int step = image.step, lstep = labels.step;
    int channels = image.channels(), lchannels = labels.channels();
    int labelcount = 1;
    DisjointSets dset;
    dset.AddElements(1);
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            if (image.data[i*step + j*channels]) {
                //If not background, check neighbors of foreground.
                int minLabel = std::numeric_limits<int>::max();
                list<Point> neighbors;
                //If neighbor is already labeled, assign minimum label value.
                //Check NW pixel.
                if (i > 0 && j > 0 && labels.data[(i-1)*lstep + (j-1)*lchannels] > 0)
                {
                    //qDebug() << "NW: " << labels.data[(i-1)*lstep + (j-1)*lchannels];
                    neighbors.push_back(Point(i-1, j-1));
                    if (labels.data[(i-1)*lstep + (j-1)*lchannels] < minLabel)
                        minLabel = labels.data[(i-1)*lstep + (j-1)*lchannels];
                    //qDebug() << "Adding NW";
                }
                //Check N pixel.
                if (i > 0 && labels.data[(i-1)*lstep + j*lchannels] > 0)
                {
                    //qDebug() << "N: " << labels.data[(i-1)*lstep + j*lchannels];
                    neighbors.push_back(Point(i-1, j));
                    if (labels.data[(i-1)*lstep + j*lchannels] < minLabel)
                        minLabel = labels.data[(i-1)*lstep + j*lchannels];
                    //qDebug() << "Adding N";
                }
                //Check NE pixel.
                if (i > 0 && j < image.cols-1 && labels.data[(i-1)*lstep + (j+1)*lchannels] > 0)
                {
                    //qDebug() << "NE: " << labels.data[(i-1)*lstep + (j+1)*lchannels];
                    neighbors.push_back(Point(i-1, j+1));
                    if (labels.data[(i-1)*lstep + (j+1)*lchannels] < minLabel)
                        minLabel = labels.data[(i-1)*lstep + (j+1)*lchannels];
                    //qDebug() << "Adding NE.";
                }
                //Check W pixel.
                if (j > 0 && labels.data[i*lstep + (j-1)*lchannels] > 0)
                {
                    //qDebug() << "W: " << labels.data[i*lstep + (j-1)*lchannels];
                    neighbors.push_back(Point(i, j-1));
                    if (labels.data[i*lstep + (j-1)*lchannels] < minLabel)
                        minLabel = labels.data[i*lstep + (j-1)*lchannels];
                    //qDebug() << "Adding W.";
                }
                if (neighbors.empty()) {
                    //If no neighbors have a label, then simply assign a new one.
                    //qDebug() << "Neighbors is empty. Adding new label: " << labelcount;
                    dset.AddElements(1);
                    //qDebug() << "Created new  label: " << dset.FindSet(labelcount);
                    labels.data[i*lstep + j*lchannels] = labelcount++;
                } else {
                    //If neighbors with labels exist, assign the smallest label to the current pixel.
                    //qDebug() << labels.data[i*lstep + j*lchannels];
                    labels.data[i*lstep + j*lchannels] = minLabel;
                    //qDebug() << "Assigning " << minLabel << " as the new label for " << i << "," << j;
                    //Then, merge all of the neighboring labels with this smallest label.
                    list<Point>::iterator it1;
                    for (it1 = neighbors.begin(); it1 != neighbors.end(); it1++) {
                        //qDebug() << "Merging " << labels.data[it1->x*lstep + it1->y*lchannels] << " with " << minLabel;
                        dset.Union(labels.data[it1->x*lstep + it1->y*lchannels], minLabel);
                    }
                }
            }
        }
    }
    //First pass is now complete. Now, iterate through labels and replace sub-labels.
    for (int i = 0; i < labels.rows; i++) {
        for (int j = 0; j < labels.cols; j++) {
            int label = labels.data[i*lstep + j*lchannels]; //Get temporary label.
            if (label > 0) {
                //qDebug() << "Acquired a > 0 label for " << i << "," << j;
                labels.data[i*lstep + j*lchannels] = dset.FindSet(label);
            }
        }
    }
    pair<Mat, int> labelPair(labels, labelcount);
    return labelPair;
}

//
Mat ProcessingThread::sizeFilter(Mat image) {
    pair<Mat, int> labelPair = extractComponentLabels(image);
    Mat labels = labelPair.first;
    int labelcount = labelPair.second;
    int step = image.step;
    int channels = image.channels();
    int lstep = labels.step;
    int lchannels = labels.channels();

    map<int, int> blobSizes;
    for (int i = 1; i < labelcount; i++) {
        blobSizes[i] = 0;
    }
    for (int i = 0; i < labels.rows; i++) {
        for (int j = 0; j < labels.cols; j++) {
            int label = labels.data[i*lstep + j*lchannels];
            if (label > 0) {
                blobSizes[label] += 1;
            }
        }
    }
    int largestBlobLabel = 1;
    for (int i = 1; i < labelcount; i++) {
        if (blobSizes[i] > blobSizes[largestBlobLabel])
            largestBlobLabel = i;
        qDebug() << i << ": " << blobSizes[i];
    }
    //Create new binary image of only the largest blob.
    Mat tempImg(image.rows, image.cols, image.type());
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            int newValue = (labels.data[i*lstep + j*lchannels] == largestBlobLabel) ? 100 : 0;
            tempImg.data[i*step + j*channels] = newValue;
            tempImg.data[i*step + j*channels+ 1] = 0;
            tempImg.data[i*step + j*channels + 2] = 0;
        }
    }
    return tempImg;
}

//Removes 1-2 long bridges.
Mat ProcessingThread::removeBridges(Mat image) {
    //Removes thin pixel bridges (horizontal, vertical) from an image.
    int step = image.step;
    int channels = image.channels();
    for (int i = 0; i < image.rows; i++) {
        for (int j = 1; j < image.cols-1; j++) {
            //Check horizontally.
            if (!image.data[i*step + (j-1)*channels] && image.data[i*step + j*channels]) {
                //Bridge of width 1.
                if (!image.data[i*step + (j+1)*channels]) {
                    //Remove one of the pixels.
                    image.data[i*step + j*channels] = 0;
                    //Update iterant.
                    j++;
                }
                if (j < image.cols-2 && image.data[i*step + (j+1)*channels] &&
                        !image.data[i*step + (j+2)*channels]) {
                    image.data[i*step + j*channels] = 0;
                    image.data[i*step + (j+1)*channels] = 0;
                    j+=2;
                }
            }
        }
    }
    return image;
}

//Converts an image to a two-color bitmap.
Mat ProcessingThread::extractBinaryMat(Mat image, QRectF frame, set<string> cluster) {
    Mat binMat = Mat::zeros(frame.height(), frame.width(), image.type());
    int i, j, x, y;
    int step = image.step;
    int channels = image.channels();
    Point3_<uchar> pixelData;
    qDebug() << "Extracting Binary Matrix from Image of : {" << image.rows << "," << image.cols << "}";
    qDebug() << "Binary Mat is of : {" << binMat.rows << "," << binMat.cols << "}";
    qDebug() << "Frame provided is of : {" << frame.height() << "," << frame.width() << "}";
    for (i = frame.top(), x = 0; i < frame.bottom(); i++, x++) {
        for (j = frame.left(), y = 0; j < frame.right(); j++, y++) {
            //qDebug() << "{" << i << "," << j << "}";
            //Iterate through the frame indices.
            //Get the pixel color at the index j, i;
            //LAB
            pixelData.x = image.data[step*i + channels*j + 0];
            pixelData.y = image.data[step*i + channels*j + 1];
            pixelData.z = image.data[step*i + channels*j + 2];

            std::string colorString = color2Hex(pixelData);
            //qDebug() << QString::fromStdString(colorString);
            if (cluster.count(colorString)) {
                binMat.data[binMat.step*x + binMat.channels()*y + 0] = 100;
            } else {
                binMat.data[binMat.step*x + binMat.channels()*y + 0] = 0;
            }
        }
    }
    //cvShowImage("BinMask", new IplImage(binMat));
    return binMat;
}

Mat ProcessingThread::extractBinaryMat(Mat image, QRectF frame, IntClusterMap *clusters, int clusterID) {
    set<std::string> cluster = (*clusters)[clusterID];
    return extractBinaryMat(image, frame, cluster);
}

//
Mat ProcessingThread::mask(Mat image, Mat mask) {
    Mat dest = Mat::zeros(image.rows, image.cols, CV_8UC3);
    //mask is guaranteed to be a bitmap (only 2 distinct values, 0 and a non 0.
    for (int i = 0; i < dest.rows; i++) {
        for (int j = 0; j < dest.cols; j++) {
            if (mask.data[mask.step*i + mask.channels()*j]) {
                dest.data[dest.step*i + dest.channels()*j] = image.data[image.step*i + image.channels()*j];
                dest.data[dest.step*i + dest.channels()*j + 1] = image.data[image.step*i + image.channels()*j + 1];
                dest.data[dest.step*i + dest.channels()*j + 2] = image.data[image.step*i + image.channels()*j + 2];
            }
        }
    }
    return dest;
}

//
QRectF ProcessingThread::fitBinRect(Mat image) {
    QRectF fittedBound;
    int fit_top = -1;
    int fit_left = std::numeric_limits<int>::max();
    int fit_right = std::numeric_limits<int>::min();
    int fit_bottom = std::numeric_limits<int>::min();
    int step = image.step;
    int channels = image.channels();
    int i, j;

    for (i = 0; i < image.rows; i++) {
        for (j = 0; j < image.cols; j++) {
            if (image.data[step*i + channels*j]) {
                if (fit_top == -1)
                    fit_top = i;
                fit_left = min(fit_left, j);
                fit_right = max(fit_right, j);
                fit_bottom = max(fit_bottom, i);
            }
        }
    }

    fittedBound.setLeft(fit_left);
    fittedBound.setTop(fit_top);
    fittedBound.setWidth(fit_right - fit_left);
    fittedBound.setHeight(fit_bottom - fit_top);

    return fittedBound;
}

//
QRectF ProcessingThread::fitRect(Mat dest, QRectF bound, IntClusterMap *clusters, int m) {
    QRectF fittedBound;
    int fit_top = -1; //Start the top as invalid. The first color that fits within the desired cluster decides the top row.
    int fit_left = std::numeric_limits<int>::max(); //Start the left as invalid. The first color that fits within the desired cluster decides the left column.
    int fit_right = std::numeric_limits<int>::min();
    int fit_bottom = std::numeric_limits<int>::min();
    int step = dest.step;
    int channels = dest.channels();
    int boundTop = bound.top();
    int boundHeight = bound.height();
    int boundLeft = bound.left();
    int boundWidth = bound.width();
    int i, j;
    //Iterate through the bounded area.
    for (i = boundTop; i < boundTop + boundHeight; i++) {
        for (j = boundLeft; j < boundLeft + boundWidth; j++) {
            //Acquire Pixel Values in L*a*b* form.
            Point3_<uchar> pixelData;
            //L*: 0-255 [0-100]
            pixelData.x = dest.data[step*i + channels*j + 0];
            //a*: 0-25(4/5?)[-127 - 127]
            pixelData.y = dest.data[step*i + channels*j + 1];
            //b*: 0-25(4/5?)[-127 - 127]
            pixelData.z = dest.data[step*i + channels*j + 2];

            //qDebug() << pixelData.x << ", " << pixelData.y << ", " << pixelData.z;

            //Check to see if color c (pixelData) at the current position (j, i) is in the accepted cluster.
            if ((*clusters)[m].count(color2Hex(pixelData))) {
                //qDebug() << QString::fromStdString(color2Hex(pixelData)) << " exists in cluster " << m;
                if (fit_top == -1) //The first chance we get, assign a row value to this.
                    fit_top = i; //After all, i represents a row (vertical) position.
                //Minimum j value at which the color exists becomes the left bound.
                fit_left = min(fit_left, j); //j represents a columnal (horizontal) position.
                fit_right = max(fit_right, j);
                fit_bottom = max(fit_bottom, i);
            }
        }
    }

    qDebug() << "Fit Left: " << fit_left;
    qDebug() << "Fit Top: " << fit_top;
    qDebug() << "Fit Right: " << fit_right;
    qDebug() << "Fit Bottom: " << fit_bottom;

    //With the new fitted box, the top() position is calculated by fit_top, the height() is calculated by fit_bottom - fit_top,
    //and the left() position is calculated by fit_left, where the width() is calculated by fit_right - fit_left.
    fittedBound.setLeft(fit_left);
    fittedBound.setTop(fit_top);
    fittedBound.setWidth(fit_right - fit_left);
    fittedBound.setHeight(fit_bottom - fit_top);

    return fittedBound;
}

//
int ProcessingThread::awkmeans(Mat image, QRectF bound, list<Point3_<uchar> > *centers, IntClusterMap *clusters, ColorFrequencyMap *colorFreqs,
                                int groupID = -1) { //groupID reflects the ID of the group cluster color to be focused on. Guaranteed to be a center.
    if (bound.isEmpty() || bound.isNull() || ! bound.isValid())
        return -1;
    int step = image.step;
    int channels = image.channels();

    int boundLeft = bound.left();
    int boundTop = bound.top();
    int boundWidth = bound.width();
    int boundHeight = bound.height();

    int n = boundWidth * boundHeight;
    int ki = sqrt((double)n/5);
    int i, j; //looping iterants
    int x = 0, y = 0; //pixel coordinate references
    list<Point3_<uchar> > prevCenters;
    list<Point3_<uchar> >::iterator it1, it2;
    Point3_<uchar> pixelData;

    if (ki <= 0)
        return -1;

    qDebug() << "n: " << n << ", " << "ki: " << ki;

    //Choose Ki (initial) evenly spaced pixels throughout the grid.
    //Spacing = n / Ki.
    //Each point is determined by the looping iterant i.

    int iStep = (boundWidth * boundHeight) / ki;
    for (i = 0; i < ki && y < boundHeight; i++) {
        if ((x+=iStep) > boundWidth) {
            //If the horizontal traversal has past the bounds, jump to the next row.
            x %= boundWidth; //Start at beginning (Left) of row.
            y++;
        }
        pixelData.x = image.data[step*(y+boundTop) + channels*(x+boundLeft) + 0];
        pixelData.y = image.data[step*(y+boundTop) + channels*(x+boundLeft) + 1];
        pixelData.z = image.data[step*(y+boundTop) + channels*(x+boundLeft) + 2];
        //Add the chosen center to the list.
        centers->push_back(pixelData);
    }

    //Add background colors to clusters.
    for(i = 0; i < backgroundPalette.cols; i++) {
        pixelData.x = backgroundPalette.data[backgroundPalette.channels()*i];
        pixelData.y = backgroundPalette.data[backgroundPalette.channels()*i + 1];
        pixelData.z = backgroundPalette.data[backgroundPalette.channels()*i + 2];
        centers->push_back(pixelData);
    }

    //If a positive (or zero) groupID was provided, add the group's representative color to the centers.
    if (groupID > -1)
        centers->push_back(int_groups[groupID]->getColorPoint());
    qDebug() << "Starting with: " << centers->size() << " clusters.";

    //Given that sqrt(n/2) clusters might be a bit excessive, "merge" similar pixel values.
    for (it1 = centers->begin(); it1 != centers->end(); it1++) {
        for (it2 = it1; it2 != centers->end();) {
            if (it2 != it1) {
                //Compare each pixel value to that of it1. If two pixels are similar
                //enough, merge them.
                double distance = colorDistance(*it1, *it2);
                if (distance < DIST_THRESH_LAB) {
                    //Determine the average of the two colors an assign the new values...
                    it1->x = (it1->x + it2->x) / 2;
                    it1->y = (it1->y + it2->y) / 2;
                    it1->z = (it1->z + it2->z) / 2;
                    //Remove it2's held value from the list and update it2.
                    it2 = centers->erase(it2);
                } else it2++;
            } else it2++;
        }
    }
    qDebug() << "Shrunk cluster centers down to: " << centers->size();

    int numKRuns = 0;
    bool kmeans = true;
    int k, m;

    while (kmeans) {
        //At the beginning of each loop, clear the clusters so they can be refilled
        //upon recalculating.
        clusters->clear();
        //Go through each pixel value.
        for (i = boundTop; i < boundTop + boundHeight; i++) {
            for (j = boundLeft; j < boundLeft + boundWidth; j++) {
                //Acquire pixel values.
                pixelData.x = image.data[step*i + channels*j + 0];
                pixelData.y = image.data[step*i + channels*j + 1];
                pixelData.z = image.data[step*i + channels*j + 2];
                //Iterate through centers and calculate the minimum Euclidean distance.
                double minDistance = std::numeric_limits<double>::max();
                for (it1 = centers->begin(), k = 0; it1 != centers->end(); it1++, k++) {
                    double distance = colorDistance(pixelData, *it1);
                    if (distance < minDistance) {
                        //Update the minimum.
                        minDistance = distance;
                        //Update m to represent the new aligned cluster.
                        m = k;
                    }
                }

                //Get a hexadecimal string representation of the pixel data.
                string hexColorString = color2Hex(pixelData);
                //Once the pixel's new cluster is determined, add its color to the
                //corresponding cluster set.
                (*clusters)[m].insert(hexColorString);

                //On the first run, map the values of every unique color c to its
                //number count.
                if (! numKRuns) {
                    (*colorFreqs)[hexColorString] += 1;
                }
            }
        }

        if (! numKRuns) {
            //On the first run, calculate the frequencies of every pixel color.
            map<std::string, float>::iterator it3;
            for (it3 = colorFreqs->begin(); it3 != colorFreqs->end(); it3++) {
                (*colorFreqs)[(*it3).first] = (*it3).second / n;
            }
        }
        numKRuns++;

        map<int, set<std::string> >::iterator it4;
        set<std::string>::iterator it5;

        //Assign current centers to old centers and recalculate new...
        prevCenters = *centers;
        //Go through all kf clusters and calculate new m for each.
        for (it4 = clusters->begin(), m = 0; it4 != clusters->end(); it4++, m++) {
            float T = 0; //Sum of all frequencies for colors.
            float tempSumX = 0, tempSumY = 0, tempSumZ = 0;
            for (it5 = (*it4).second.begin(); it5 != (*it4).second.end(); it5++) {
                //Iterate through all colors in the current cluster.
                T += (*colorFreqs)[*it5];
                //Each color is represented by a vector, so scalar operations are
                //distributed.
                pixelData = hex2Color(*it5);
                tempSumX += (*colorFreqs)[*it5]*pixelData.x;
                tempSumY += (*colorFreqs)[*it5]*pixelData.y;
                tempSumZ += (*colorFreqs)[*it5]*pixelData.z;
            }
            //Compute the weighted mean.
            tempSumX /= T;
            tempSumY /= T;
            tempSumZ /= T;

            //Replace m value in centers with newly calculated mean.
            for (it1 = centers->begin(), k = 0; it1 != centers->end(); it1++, k++) {
                if (k == m) {
                    it1->x = (uchar)tempSumX;
                    it1->y = (uchar)tempSumY;
                    it1->z = (uchar)tempSumZ;
                    break;
                }
            }
        }
        kmeans = false;
        for (it1 = centers->begin(), it2 = prevCenters.begin(); it1 != centers->end(); it1++, it2++) {
            //Iterate through all the centers. If m(t) = m(t-1) for all m, end loop.
            if (it1->x != it2->x || it1->y != it2->y || it1->z != it2->z) {
                kmeans = true;
                break;
            }
        }
    }

    return numKRuns+1;
}

//
void ProcessingThread::updateSubjects() {
    imageData->getReadSlot();
    imageData->updateGroups(int_groups);
    imageData->releaseWriteSlot();
}

//Returns the Matrix form of the current Frame held.
Mat ProcessingThread::getCurrentFrame() {
    frameProtectMutex.lock();
    Mat clone = currentFrame.clone();
    frameProtectMutex.unlock();
    return clone;
}

//Returns the index/number of the current Frame held.
int ProcessingThread::getCurrentFrameIndex() {
    return currentIndex;
}

Point3_<uchar> ProcessingThread::setGroupColor(QPoint pos, int ID) {
    groupsMutex.lock();
    frameProtectMutex.lock();

    Point3_<uchar> rgbCol;
    rgbCol.x = currentFrame.data[currentFrame.step*pos.y() + currentFrame.channels()*pos.x() + 0];
    rgbCol.y = currentFrame.data[currentFrame.step*pos.y() + currentFrame.channels()*pos.x() + 1];
    rgbCol.z = currentFrame.data[currentFrame.step*pos.y() + currentFrame.channels()*pos.x() + 2];

    Mat tempDest;
    cvtColor(currentFrame, tempDest, CV_BGR2Lab);
    frameProtectMutex.unlock();

    Point3_<uchar> labCol;
    labCol.x = tempDest.data[tempDest.step*pos.y() + tempDest.channels()*pos.x() + 0];
    labCol.y = tempDest.data[tempDest.step*pos.y() + tempDest.channels()*pos.x() + 1];
    labCol.z = tempDest.data[tempDest.step*pos.y() + tempDest.channels()*pos.x() + 2];

    std::string hexCol = color2Hex(labCol);

    if (int_groups.count(ID)) { //Already contains key
        int_groups[ID]->setColor(labCol); //Groups maps keys to group pointers.
        //No need to alter the int_groups as it contains the pointer.
    }
    else {
       SubjectGroup* tempGroup = new SubjectGroup(labCol, ID);
       groups[hexCol] = tempGroup;
       int_groups[ID] = tempGroup;
    }
    groupsMutex.unlock();
    return rgbCol;
}

void ProcessingThread::removeGroup(int ID) {
    groupsMutex.lock();
    string tempHex = int_groups[ID]->getColorString();
    groups.erase(tempHex);
    int_groups.erase(ID);
    groupsMutex.unlock();
}

SubjectGroup* ProcessingThread::getSubjectGroup(Point3_<uchar> color) {
    groupsMutex.lock();
    SubjectGroup* tempGroup = groups[color2Hex(color)];
    groupsMutex.unlock();
    return tempGroup;
}

SubjectGroup* ProcessingThread::getSubjectGroup(int ID) {
    groupsMutex.lock();
    SubjectGroup* tempGroup = int_groups[ID];
    groupsMutex.unlock();
    return tempGroup;
}

Subject* ProcessingThread::getSubject(int subjectID, int groupID) {
    map<int, Subject*>::iterator it;
    SubjectGroup* subjectGroup = int_groups[groupID];
    IntSubjectMap subjects = subjectGroup->getSubjects();
    Subject* subj;
    for (it = subjects.begin(); it != subjects.end(); it++) {
        if (it->second->getID() == subjectID)
        {
            subj = it->second;
            break;
        }
    }
    return subj;
}

void ProcessingThread::dropFrame() {
    currentFrame.release();
    currentIndex = -1;
}

//Stops the Processing thread.
void ProcessingThread::stopProcessingThread() {
    stoppedMutex.lock();
        stopped = true;
    stoppedMutex.unlock();
    imageHandler->releaseProcSlot();
    //this->run();
}

////Create a display reflecting the palette's colors.
void ProcessingThread::showPalette(list<Point3_<uchar> > palette, char* windowName, int squareSize) {
    list<Point3_<uchar> >::iterator it;
    int i;
    IplImage* tempImg;

    tempImg = cvCreateImage(cvSize(palette.size()*squareSize, squareSize), 8, 3);
    for (it = palette.begin(), i = 0; it != palette.end(); it++, i++) {
        cvRectangle(tempImg, cvPoint(i*squareSize, 0), cvPoint(i*squareSize + squareSize, squareSize), cvScalar(it->x, it->y, it->z), -1);
    }
    cvShowImage(windowName, tempImg);
}

