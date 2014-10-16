#include "DisplayThread.h"
#include "VideoFrame.h"
#include <QDebug>
#include "Subject.h"
#include "SubjectGroup.h"

DisplayThread::DisplayThread(ImageData* imageData) :
    QThread(), imageData(imageData), drawBox(false), subjectBox(-1)
{
    stopped = false;

    currentIndex = -1;

    this->imageData = imageData;

    this->mouseData.cursorType = DEFAULT;

    startPoint.setX(0);
    startPoint.setY(0);

//    cvNamedWindow("Color Picker");
}

void DisplayThread::run() {
    qDebug() << "Display thread started...";

    while(1) {

        qDebug() << "Display Thread: Waiting for write slot.";

        imageData->getWriteSlot();

        qDebug() << "Display Thread: Acquired write slot.";

        //STOP CHECK
        stoppedMutex.lock();
        if (stopped) {
            stopped = false;
            stoppedMutex.unlock();
            break;
        } stoppedMutex.unlock();
        //END STOP CHECK

        qDebug() << "Display Thread: Locking frameProtect.";
        frameProtectMutex.lock();
        currentFrame = imageData->getFrame();
        currentIndex = imageData->currentIndex();
        groups = imageData->getGroups();

        qDebug() << "Display Thread: Unlocking frameProtect.";
        frameProtectMutex.unlock();

        qDebug() << "Display Thread: Releasing read slot.";
        imageData->releaseReadSlot();

        imageProtectMutex.lock();
        sourceImage = MatToQImage(currentFrame);
        paint();
        imageProtectMutex.unlock();
    } qDebug() << "Stopping Display Thread...";
    imageData->stop();
}

void DisplayThread::paint() {
    paintProtectMutex.lock();
    qDebug() << "Display Thread: painting...";
    modImage = sourceImage;
    QPainter painter(&modImage);

    if (drawBox) {
        painter.setPen(Qt::cyan);
        painter.drawRect(*selectionBox);
    }
    if (subjectBox > -1) {
        if (groups.count(subjectBox)) {
            painter.setPen(Qt::cyan);
            //Iterate through subjects map from groups.
            IntSubjectMap subjects = groups[subjectBox]->getSubjects();

            map<int, Subject*>::iterator it1;

            for (it1 = subjects.begin(); it1 != subjects.end(); it1++) {
                painter.setPen(Qt::darkCyan);
                //Update mod image and paint each qRectangle.
                QRectF frame = it1->second->getCurrentBoundingFrame();
                painter.drawRect(frame);
                QPointF pt1 = it1->second->pos();
                //SHIFT FROM ORIGIN 0,0 TO FRAME BOUNDS
                painter.setPen(Qt::yellow);
                //pt1.setX(pt1.x() + frame.left());
                //pt1.setY(pt1.y() + frame.top());
                painter.drawLine(QPointF(pt1.x(), pt1.y()),
                                 QPointF(pt1.x() + 15*sin(it1->second->dir()), pt1.y() - 15*cos(it1->second->dir())));
            }
        }
    }
    painter.end();
    paintProtectMutex.unlock();
    emit frameUpdate(modImage, currentIndex);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\\
//%%%%%%%%%% MOUSE UTILITIES %%%%%%%%%%\\
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\\

void DisplayThread::onMouseMove(QMouseEvent *ev) {
    mouseProtectMutex.lock();

    mouseData.pos = ev->pos();

    frameProtectMutex.lock();
    if (mouseData.pos.x() > currentFrame.cols)
        mouseData.pos.setX(currentFrame.cols);
    else if (mouseData.pos.x() < 0)
        mouseData.pos.setX(0);
    if (mouseData.pos.y() > currentFrame.rows)
        mouseData.pos.setY(currentFrame.rows);
    else if (mouseData.pos.y() < 0)
        mouseData.pos.setY(0);
    frameProtectMutex.unlock();

    if (drawBox) {
        //qDebug() << "Draw Box on!";
        selectionBox->setWidth(mouseData.pos.x()-startPoint.x());
        selectionBox->setHeight(mouseData.pos.y()-startPoint.y());

        if (selectionBox->width() > 50) selectionBox->setWidth(50);
        if (selectionBox->width() < -50) selectionBox->setWidth(-50);
        if (selectionBox->height() > 50) selectionBox->setHeight(50);
        if (selectionBox->height() < -50) selectionBox->setHeight(-50);

        imageProtectMutex.lock();
        this->paint();
        imageProtectMutex.unlock();
    }

    mouseProtectMutex.unlock();
}

void DisplayThread::onMousePress(QMouseEvent *ev) {
    bool flag = false;
    mouseProtectMutex.lock();

    mouseData.pos = ev->pos();

    if (mouseData.cursorType == SUBJECT_SELECTOR) {
        if (ev->button() == Qt::LeftButton) {
            startPoint = ev->pos();
            selectionBox = new QRect(startPoint.x(), startPoint.y(), 0, 0);
            drawBox = true;
            imageProtectMutex.lock();
            this->paint();
            imageProtectMutex.unlock();
        }
    } else if (mouseData.cursorType == COLOR_PICKER) {
        mouseProtectMutex.unlock();
        flag = true;
        emit colorPicked(mouseData.pos);
    }
    if (!flag)
        mouseProtectMutex.unlock();
}

void DisplayThread::onMouseRelease(QMouseEvent *ev) {
    mouseProtectMutex.lock();

    mouseData.pos = ev->pos();

    if (mouseData.cursorType == SUBJECT_SELECTOR){

        if (ev->button() == Qt::LeftButton) {
            mouseData.leftButtonRelease = true;
            if (drawBox) {
                drawBox = false;
                mouseData.selectionBox.setX(this->selectionBox->left());
                mouseData.selectionBox.setY(this->selectionBox->top());
                mouseData.selectionBox.setWidth(this->selectionBox->width());
                mouseData.selectionBox.setHeight(this->selectionBox->height());

                mouseData.leftButtonRelease = true;

                emit selectionBoxFormed(mouseData);
            }
        }
        else if (ev->button() == Qt::RightButton) {
            if (drawBox) drawBox = false;
        }
    }

    mouseProtectMutex.unlock();
}

void DisplayThread::dropFrame() {
    frameProtectMutex.lock();

    currentFrame.release();
    currentIndex = -1;

    frameProtectMutex.unlock();
}

void DisplayThread::stopDisplayThread() {
    stoppedMutex.lock();
    stopped = true;
    stoppedMutex.unlock();
    imageData->releaseWriteSlot();
}

void DisplayThread::setMouseCursor(int cursorType) {
    mouseProtectMutex.lock();
    mouseData.cursorType = cursorType;
    mouseProtectMutex.unlock();
}

QPoint DisplayThread::getMouseCursorPos() {
    mouseProtectMutex.lock();
    QPoint tempPoint = this->mouseData.pos;
    mouseProtectMutex.unlock();
    return tempPoint;
}

int DisplayThread::getMouseCursorType() {
    mouseProtectMutex.lock();

    int tempType = mouseData.cursorType;

    mouseProtectMutex.unlock();

    return tempType;
}

int DisplayThread::getCurrentFrameIndex() {
    frameProtectMutex.lock();
    int tempIndex = currentIndex;
    frameProtectMutex.unlock();
    return tempIndex;
}

Mat DisplayThread::getCurrentSourceFrame() {
    frameProtectMutex.lock();
    Mat sourceFrame = currentFrame.clone();
    frameProtectMutex.unlock();
    return sourceFrame;
}

void DisplayThread::releaseSelectionBox() {
    mouseProtectMutex.lock();

    mouseData.selectionBox.setX(0);
    mouseData.selectionBox.setY(0);
    mouseData.selectionBox.setWidth(0);
    mouseData.selectionBox.setHeight(0);

    this->paint();

    mouseProtectMutex.unlock();
}

void DisplayThread::showSubjectBoxes(const int groupID) {
    flagProtectMutex.lock();
    this->subjectBox = groupID;
    flagProtectMutex.unlock();
    this->paint();
}
