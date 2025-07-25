#pragma once

#include "seer/viewerbase.h"

class F3DWidget;
class SidebarWnd;

class F3DViewer : public ViewerBase {
    Q_OBJECT
public:
    explicit F3DViewer(QWidget* parent = nullptr);
    ~F3DViewer() override;

    QString name() const override
    {
        return "F3DViewer";
    }

    QSize getContentSize() const override;
    void updateDPR(qreal) override;
    void updateTheme(int) override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

    void loadImpl(QBoxLayout* lay_content, QHBoxLayout* lay_ctrlbar) override;

private:
    void initSidebar();
    void syncSidebar();

    QToolButton* m_btn    = nullptr;
    SidebarWnd* m_sidebar = nullptr;
    F3DWidget* m_view     = nullptr;
};

class F3DPlugin : public QObject, public ViewerPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ViewerPluginInterface_iid FILE "bin/plugin.json")
    Q_INTERFACES(ViewerPluginInterface)
public:
    ViewerBase* createViewer(QWidget* parent = nullptr) override
    {
        return new F3DViewer(parent);
    }
};
