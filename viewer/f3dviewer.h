#pragma once

#include <qt_windows.h>

#include "seer/viewerbase.h"

class QProcess;

class F3DViewer : public ViewerBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ViewerBase_iid FILE "F3DViewer.json")
    Q_INTERFACES(ViewerBase)
public:
    explicit F3DViewer(QWidget* parent = nullptr);
    ~F3DViewer() override;

    QString name() const override
    {
        return "F3DViewer";
    }

    QSize getContentSize() const override;
    void updateDPR(qreal) override;

protected:
    void loadImpl(QBoxLayout* lay_content, QHBoxLayout* lay_ctrlbar) override;
    void resizeEvent(QResizeEvent* event) override;

private:
};
