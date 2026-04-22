#pragma once

#include "seer/viewerbase.h"

class F3DWidget;
class SidebarWnd;
class QToolButton;
class QSettings;

class F3DViewer : public ViewerBase {
    Q_OBJECT
public:
    explicit F3DViewer(QWidget *parent = nullptr);
    ~F3DViewer() override;

    QString name() const override
    {
        return "f3dviewer";
    }

    QSize getContentSize() const override;
    void updateDPR(qreal) override;
    void updateTheme(int) override;

protected:
    void keyPressEvent(QKeyEvent *event) override;

    void loadImpl(QBoxLayout *lay_content, QHBoxLayout *lay_ctrlbar) override;

private:
    void initSidebar();
    void syncSidebar();
    void saveIni();
    void saveDisplayIni();
    void setSidebarVisible(bool visible);
    void resetViewOptions();
    QString getIniPath() const;

    QSettings *m_ini            = nullptr;
    QToolButton *m_btn          = nullptr;
    SidebarWnd *m_sidebar       = nullptr;
    F3DWidget *m_view           = nullptr;
    bool m_sidebar_visible_pref = true;
    bool m_options_ready        = false;
};

class F3DPlugin : public QObject, public ViewerPluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ViewerPluginInterface_iid FILE "bin/plugin.json")
    Q_INTERFACES(ViewerPluginInterface)
public:
    ViewerBase *createViewer(QWidget *parent = nullptr) override
    {
        return new F3DViewer(parent);
    }
};
