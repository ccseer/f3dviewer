#include "f3dviewer.h"

#define qprintt qDebug() << "[F3DViewer]"

F3DViewer::F3DViewer(QWidget* parent) : ViewerBase(parent)
{
    qprintt << this;
}

F3DViewer::~F3DViewer()
{
    qprintt << "~" << this;
}

QSize F3DViewer::getContentSize() const {}

void F3DViewer::updateDPR(qreal qreal)
{
    m_d->d->dpr = qreal;
    // "ui.scale";
}