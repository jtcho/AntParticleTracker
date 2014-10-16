#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include "Structures.h"
#include <QTGui>
#include <QLabel>

class VideoFrame : public QLabel
{
    Q_OBJECT
public:
    VideoFrame(QWidget *parent = 0);
private:
protected:
    void mouseMoveEvent(QMouseEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void paintEvent(QPaintEvent *ev);
signals:
    void onMouseMoveEvent(QMouseEvent* ev);
    void onMousePressEvent(QMouseEvent* ev);
    void onMouseReleaseEvent(QMouseEvent* ev);
};

#endif // VIDEOFRAME_H
