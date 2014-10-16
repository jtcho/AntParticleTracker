#include "VideoFrame.h"
#include <QDebug>

VideoFrame::VideoFrame(QWidget *parent) : QLabel(parent)
{
}

void VideoFrame::mouseMoveEvent(QMouseEvent *ev) {
    emit onMouseMoveEvent(ev);
}

void VideoFrame::mouseReleaseEvent(QMouseEvent *ev) {
    emit onMouseReleaseEvent(ev);
}

void VideoFrame::mousePressEvent(QMouseEvent *ev) {
    emit onMousePressEvent(ev);
}

void VideoFrame::paintEvent(QPaintEvent *ev) {
    QLabel::paintEvent(ev);
}
