#include "f3dviewer.h"

#include <QApplication>
#include <QKeyEvent>

#include "F3DWidget.h"

#define qprintt qDebug() << "[F3DViewer]"

F3DViewer::F3DViewer(QWidget* parent) : ViewerBase(parent)
{
    qprintt << this;
}

F3DViewer::~F3DViewer()
{
    qprintt << "~" << this;
}

QSize F3DViewer::getContentSize() const
{
    return m_d->d->dpr * QSize{800, 800};
}

void F3DViewer::updateDPR(qreal r)
{
    m_d->d->dpr = r;
    // "ui.scale";
}

void F3DViewer::updateTheme(int t)
{
    m_d->d->theme = t;
    auto bg       = qApp->palette().color(QPalette::Window);
    if (m_view) {
        m_view->setOption("render.background.color", QString("%1,%2,%3")
                                                         .arg(bg.redF())
                                                         .arg(bg.greenF())
                                                         .arg(bg.blueF()));
    }
}

void F3DViewer::keyPressEvent(QKeyEvent* event)
{
    // ViewerBase::keyPressEvent(event);
    if (m_view) {
        qApp->sendEvent(m_view, event);
    }
}

void F3DViewer::loadImpl(QBoxLayout* lay_content, QHBoxLayout* lay_ctrlbar)
{
    m_view = new F3DWidget(this);
    lay_content->addWidget(m_view);
    if (!m_view->load(m_d->d->path)) {
        emit sigCommand(ViewCommandType::VCT_StateChange, VCV_Error);
        return;
    }
    if (lay_ctrlbar) {
        lay_ctrlbar->addStretch();
    }

    updateTheme(m_d->d->theme);
    updateDPR(m_d->d->dpr);

    emit sigCommand(ViewCommandType::VCT_StateChange, VCV_Loaded);
}
