#include "sidebarwnd.h"

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSlider>
#include <QTextDocument>
#include <QToolButton>

#include "ui_sidebarwnd.h"

SidebarWnd::SidebarWnd(QWidget *parent)
    : QWidget(parent), ui(new Ui::SidebarWnd)
{
    ui->setupUi(this);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    ui->scrollArea->setFrameStyle(QFrame::NoFrame);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->verticalScrollBar()->setProperty("is_readonly", true);

    updateAnimationPlayBtnText();
    ui->pushButton_ani_reset->setVisible(false);
    ui->slider_ani_progress->setEnabled(false);
    ui->label_ani_progress_val->setText("0.0 / 0.0");
    ui->label_render_opacity_val->setText("100%");
    ui->widget_keys_content->setVisible(false);
    ui->toolButton_keys_toggle->setAutoRaise(true);
    ui->toolButton_keys_toggle->setStyleSheet(
        "QToolButton {"
        " color: palette(buttontext);"
        " font-size: 11px;"
        " padding: 1px 6px;"
        " border: 1px solid palette(midlight);"
        " border-radius: 8px;"
        " background: palette(base);"
        " }"
        "QToolButton:hover {"
        " color: palette(buttontext);"
        " border-color: palette(buttontext);"
        " background: palette(button);"
        " }"
        "QToolButton:checked {"
        " color: palette(buttontext);"
        " border-color: palette(buttontext);"
        " background: palette(button);"
        " }"
        "QToolButton:pressed {"
        " padding: 1px 6px;"
        " border: 1px solid palette(buttontext);"
        " background: palette(midlight);"
        " }");
    ui->toolButton_keys_toggle->setText("Show");

    initKeys();
    renderKeys(devicePixelRatioF());
    updateCameraAxisLabels(true);

    for (auto *button : findChildren<QAbstractButton *>()) {
        button->setFocusPolicy(Qt::NoFocus);
    }
    for (auto *combo : findChildren<QComboBox *>()) {
        combo->setFocusPolicy(Qt::NoFocus);
    }
    for (auto *slider : findChildren<QSlider *>()) {
        slider->setFocusPolicy(Qt::NoFocus);
    }

    connect(ui->pushButton_camera_reset, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraReset);
    connect(ui->pushButton_camera_font, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraFront);
    connect(ui->pushButton_camera_back, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraBack);
    connect(ui->pushButton_camera_left, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraLeft);
    connect(ui->pushButton_camera_right, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraRight);
    connect(ui->pushButton_camera_top, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraTop);
    connect(ui->pushButton_camera_btm, &QPushButton::clicked, this,
            &SidebarWnd::sigCameraBottom);

    connect(ui->comboBox_camera_up,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (m_syncing) {
                    return;
                }
                const bool yUp = index == 0;
                updateCameraAxisLabels(yUp);
                emit sigYUpChanged(yUp);
            });
    connect(ui->checkBox_camera_orthographic, &QCheckBox::clicked, this,
            &SidebarWnd::sigOrthographicChanged);

    connect(ui->checkBox_display_axis, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowAxis);
    connect(ui->checkBox_display_grid, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowGrid);
    connect(ui->checkBox_display_edge, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowEdge);
    connect(ui->checkBox_display_ponit_sprites, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowPointSprites);
    connect(ui->checkBox_display_scalar_bar, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowScalarBar);
    connect(ui->checkBox_display_fps, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowFPS);
    connect(ui->checkBox_display_metadata, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowMetadata);

    connect(ui->checkBox_render_anti_aliasing, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowAntiAliasing);
    connect(ui->checkBox_render_ambient_occlusion, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowAmbientOcclusion);
    connect(ui->checkBox_render_tone_mapping, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowToneMapping);
    connect(ui->checkBox_render_translucency, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowTranslucencySupport);
    connect(ui->checkBox_render_hdri_ambient, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowHdriAmbient);
    connect(ui->checkBox_render_skybox, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowSkybox);
    connect(ui->checkBox_render_volume, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowVolumeRendering);
    connect(ui->checkBox_render_background_blur, &QCheckBox::clicked, this,
            &SidebarWnd::sigShowBackgroundBlur);
    connect(ui->pushButton_render_reset, &QPushButton::clicked, this,
            &SidebarWnd::sigResetViewOptions);

    connect(
        ui->slider_render_opacity, &QSlider::valueChanged, this,
        [this](int value) {
            ui->label_render_opacity_val->setText(QString("%1%").arg(value));
            if (!m_syncing) {
                emit sigOpacityChanged(value / 100.0);
            }
        });

    connect(ui->comboBox_ani_clip,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                if (!m_syncing) {
                    emit sigAnimationSelectionChanged(index - 1);
                }
            });
    connect(ui->checkBox_ani_loop, &QCheckBox::clicked, this,
            &SidebarWnd::sigAnimationLoopChanged);
    connect(ui->slider_ani_progress, &QSlider::sliderPressed, this,
            [this]() { ui->slider_ani_progress->setEnabled(true); });
    connect(ui->slider_ani_progress, &QSlider::sliderReleased, this, [this]() {
        if (m_syncing) {
            return;
        }
        emit sigSeekAnimation(ui->slider_ani_progress->value() / 1000.0);
    });

    connect(ui->slider_ani_speed, &QSlider::valueChanged, this,
            [this](int value) {
                double speed = value / 100.0;
                ui->label_ani_speed_val->setText(
                    QString("%1x").arg(speed, 0, 'f', 1));
                if (!m_syncing) {
                    emit sigAnimationSpeedChanged(speed);
                }
            });
    connect(ui->toolButton_keys_toggle, &QToolButton::toggled, this,
            [this](bool expanded) {
                ui->widget_keys_content->setVisible(expanded);
                ui->toolButton_keys_toggle->setText(expanded ? "Hide" : "Show");
            });
}

SidebarWnd::~SidebarWnd()
{
    delete ui;
}

void SidebarWnd::updateDPR(qreal r)
{
    setFixedWidth(r * 240);
    const auto list
        = {ui->label_camera_reset, ui->label_camera_up,
           ui->label_camera_front_back_axis, ui->label_camera_left_right_axis,
           ui->label_camera_top_bottom_axis};
    int w = 0;
    for (auto i : list) {
        w = qMax(w, i->fontMetrics().horizontalAdvance(i->text()));
    }
    for (auto i : list) {
        i->setFixedWidth(w);
    }
    auto font = qApp->font();
    font.setPixelSize(14 * r);
    font.setBold(true);
    for (auto i : findChildren<QLabel *>()) {
        if (i->objectName().startsWith("label_title_")) {
            i->setFont(font);
        }
    }

    w      = r * 75;
    auto h = r * 25;
    for (auto i : findChildren<QPushButton *>()) {
        i->setFixedSize(w, h);
    }

    const int ani_label_w
        = qMax(ui->label_ani_progress->fontMetrics().horizontalAdvance(
                   ui->label_ani_progress->text()),
               qMax(ui->label_ani_speed->fontMetrics().horizontalAdvance(
                        ui->label_ani_speed->text()),
                    ui->label_ani_clip->fontMetrics().horizontalAdvance(
                        ui->label_ani_clip->text())));
    ui->label_ani_progress->setFixedWidth(ani_label_w);
    ui->label_ani_speed->setFixedWidth(ani_label_w);
    ui->label_ani_clip->setFixedWidth(ani_label_w);

    const int ani_value_w = qMax(
        ui->label_ani_progress_val->fontMetrics().horizontalAdvance(
            "99.9 / 99.9"),
        qMax(ui->label_ani_speed_val->fontMetrics().horizontalAdvance("999.9x"),
             ui->label_render_opacity_val->fontMetrics().horizontalAdvance(
                 "100%")));
    ui->label_ani_progress_val->setFixedWidth(ani_value_w);
    ui->label_ani_speed_val->setFixedWidth(ani_value_w);
    ui->label_render_opacity_val->setFixedWidth(ani_value_w);

    auto smallFont = qApp->font();
    smallFont.setPixelSize(qRound(12 * r));
    ui->toolButton_keys_toggle->setFont(smallFont);
    renderKeys(r);

    QTextDocument doc;
    doc.setDefaultFont(ui->label_keys->font());
    doc.setHtml(ui->label_keys->text());
    ui->label_keys->setMinimumHeight(qRound(doc.size().height()));
}

void SidebarWnd::syncControls(const State &state)
{
    m_syncing = true;
    ui->checkBox_display_axis->setChecked(state.axis);
    ui->checkBox_display_grid->setChecked(state.grid);
    ui->checkBox_display_edge->setChecked(state.edge);
    ui->checkBox_display_ponit_sprites->setChecked(state.pointSprites);
    ui->checkBox_display_scalar_bar->setChecked(state.scalarBar);
    ui->checkBox_display_metadata->setChecked(state.metadata);
    ui->checkBox_display_fps->setChecked(state.fps);

    ui->checkBox_render_anti_aliasing->setChecked(state.antiAliasing);
    ui->checkBox_render_ambient_occlusion->setChecked(state.ambientOcclusion);
    ui->checkBox_render_tone_mapping->setChecked(state.toneMapping);
    ui->checkBox_render_translucency->setChecked(state.translucencySupport);
    ui->checkBox_render_hdri_ambient->setChecked(state.hdriAmbient);
    ui->checkBox_render_skybox->setChecked(state.skybox);
    ui->checkBox_render_volume->setChecked(state.volumeRendering);
    ui->checkBox_render_background_blur->setChecked(state.backgroundBlur);
    ui->slider_render_opacity->setValue(state.opacityPercent);
    ui->checkBox_camera_orthographic->setChecked(state.orthographic);
    ui->comboBox_camera_up->setCurrentIndex(state.yUp ? 0 : 1);
    updateCameraAxisLabels(state.yUp);

    ui->widget_grp_animation->setVisible(state.animationVisible);
    ui->comboBox_ani_clip->setCurrentIndex(state.animationSelection + 1);
    ui->checkBox_ani_loop->setChecked(state.animationLoop);
    ui->slider_ani_progress->setEnabled(state.animationVisible);
    ui->slider_ani_speed->setValue(qRound(state.animationSpeed * 100.0));
    ui->label_ani_speed_val->setText(
        QString("%1x").arg(state.animationSpeed, 0, 'f', 1));
    m_ani_run = state.animationRunning;
    updateAnimationPlayBtnText();
    m_syncing = false;
}

void SidebarWnd::setAnimationList(const QStringList &names, int currentIndex)
{
    m_syncing = true;
    ui->comboBox_ani_clip->clear();
    ui->comboBox_ani_clip->addItem("All Animations");
    for (const auto &name : names) {
        ui->comboBox_ani_clip->addItem(name);
    }
    ui->comboBox_ani_clip->setCurrentIndex(currentIndex + 1);
    m_syncing = false;
}

void SidebarWnd::updateAnimationProgress(double current, double duration)
{
    const int max = qMax(1, qRound(duration * 1000.0));
    const int val = qBound(0, qRound(current * 1000.0), max);
    {
        const QSignalBlocker blocker(ui->slider_ani_progress);
        ui->slider_ani_progress->setRange(0, max);
        if (!ui->slider_ani_progress->isSliderDown()) {
            ui->slider_ani_progress->setValue(val);
        }
    }
    ui->label_ani_progress_val->setText(
        QString("%1 / %2").arg(current, 0, 'f', 1).arg(duration, 0, 'f', 1));
}

void SidebarWnd::on_pushButton_ani_play_clicked()
{
    m_ani_run = !m_ani_run;
    updateAnimationPlayBtnText();
    emit sigPlayAnimation(m_ani_run);
}

void SidebarWnd::on_pushButton_ani_reset_clicked()
{
    m_ani_run = false;
    updateAnimationPlayBtnText();
    emit sigResetAnimationPos();
}

void SidebarWnd::updateAnimationPlayBtnText()
{
    ui->pushButton_ani_play->setText(m_ani_run ? "Pause" : "Play");
}

void SidebarWnd::updateCameraAxisLabels(bool yUp)
{
    ui->label_camera_front_back_axis->setText(yUp ? "z" : "y");
    ui->label_camera_left_right_axis->setText("x");
    ui->label_camera_top_bottom_axis->setText(yUp ? "y" : "z");
}

void SidebarWnd::initKeys()
{
    ui->label_keys->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->label_keys->setWordWrap(true);
    ui->label_keys->setTextFormat(Qt::RichText);
}

void SidebarWnd::renderKeys(qreal dpr)
{
    const auto textColor = palette().color(QPalette::Text).name(QColor::HexRgb);
    const auto keyColor  = palette().color(QPalette::Link).name(QColor::HexRgb);
    const auto sectionColor
        = palette().color(QPalette::PlaceholderText).name(QColor::HexRgb);
    const auto introColor = palette().color(QPalette::Mid).name(QColor::HexRgb);

    const QVector<QPair<QString, QString>> mouse
        = {{"Left Button Drag", "Rotate camera"},
           {"Shift + Left Button Drag", "Fine rotation"},
           {"Ctrl + Left Button Drag", "Roll camera around view axis"},
           {"Right Button Drag", "Pan camera"},
           {"Shift + Right Button Drag", "Fine panning"},
           {"Mouse Wheel Scroll", "Zoom in or out"},
           {"Shift + Mouse Wheel Scroll", "Fine zoom"},
           {"Double Click Left Button", "Reset camera to default view"}};
    const QVector<QPair<QString, QString>> key
        = {{"1-6", "Switch camera views"},
           {"Ctrl+C", "Copy current view"},
           {"B", "Toggle scalar bar"},
           {"Ctrl+P", "Increase model transparency"},
           {"Shift+P", "Decrease model transparency"},
           {"P", "Toggle translucency effect"},
           {"A", "Toggle anti-aliasing"},
           {"Shift+A", "Toggle skeleton display"},
           {"T", "Toggle tone mapping"},
           {"E", "Toggle model edges"},
           {"G", "Toggle ground grid"},
           {"M", "Toggle metadata display"},
           {"Z", "Toggle FPS counter"},
           {"V", "Toggle volume rendering"},
           {"I", "Invert volume rendering"},
           {"O", "Toggle point sprites"},
           {"U", "Toggle background blur"},
           {"Q", "Toggle ambient occlusion"},
           {"F", "Toggle ambient lighting"},
           {"J", "Toggle skybox background"},
           {"L", "Increase light intensity"},
           {"Shift+L", "Decrease light intensity"},
           {"Enter", "Reset camera view"},
           {"Tab", "Toggle sidebar"}};

    const int section_px  = qRound(12 * dpr);
    const int key_px      = qRound(12 * dpr);
    const int body_px     = qRound(13 * dpr);
    const int intro_px    = qRound(12 * dpr);
    const int card_gap    = qMax(10, qRound(10 * dpr));
    const int line_gap    = qMax(1, qRound(1 * dpr));
    const int section_gap = qMax(8, qRound(8 * dpr));
    const int intro_gap   = qMax(8, qRound(8 * dpr));

    auto buildSection =
        [](const QString &title, const QVector<QPair<QString, QString>> &items,
           int section_px, int key_px, int body_px, int card_gap, int line_gap,
           const QString &sectionColor, const QString &keyColor,
           const QString &textColor) {
            QString html
                = QString(
                      "<div style='margin-top:2px; margin-bottom:4px;'>"
                      "<span style='font-size:%1px; letter-spacing:0.08em; "
                      "text-transform:uppercase; color:%3;'>%2</span>"
                      "</div>"
                      "<table cellspacing='0' cellpadding='0' width='100%'>")
                      .arg(section_px)
                      .arg(title.toHtmlEscaped())
                      .arg(sectionColor);
            for (const auto &item : items) {
                html.append(QString("<tr><td style='font-family:Consolas, "
                                    "\"Cascadia Mono\", monospace; "
                                    "font-size:%1px; color:%7;'>%2</td></tr>"
                                    "<tr><td style='padding-top:%3px; "
                                    "font-size:%4px; color:%8;'>%5</td></tr>"
                                    "<tr><td height='%6'></td></tr>")
                                .arg(key_px)
                                .arg(item.first.toHtmlEscaped())
                                .arg(line_gap)
                                .arg(body_px)
                                .arg(item.second.toHtmlEscaped())
                                .arg(card_gap)
                                .arg(keyColor)
                                .arg(textColor));
            }
            html.append("</table>");
            return html;
        };

    QString text;
    text.append("<div style='line-height:1.2'>");
    text.append(
        QString("<div style='margin-bottom:%1px; font-size:%2px; color:%3;'>")
            .arg(intro_gap)
            .arg(intro_px)
            .arg(introColor));
    text.append(
        "Practical shortcuts for quick navigation and display control.");
    text.append("</div>");
    text.append(buildSection("Mouse", mouse, section_px, key_px, body_px,
                             card_gap, line_gap, sectionColor, keyColor,
                             textColor));
    text.append(QString("<div style='height:%1px'></div>").arg(section_gap));
    text.append("<br/>");
    text.append(buildSection("Keyboard", key, section_px, key_px, body_px,
                             card_gap, line_gap, sectionColor, keyColor,
                             textColor));
    text.append("</div>");

    ui->label_keys->setText(text);
    ui->label_keys->adjustSize();
}
