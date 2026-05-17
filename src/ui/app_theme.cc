#include "ui/app_theme.h"

#include <QtCore/QVariantMap>
#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>

namespace pfd::ui {

EffectiveUiTheme UiTheme::resolveEffectiveTheme(const QString& storedPreference) {
  const QString p = storedPreference.trimmed().toLower();
  if (p == QStringLiteral("dark")) {
    return EffectiveUiTheme::Dark;
  }
  if (p == QStringLiteral("light")) {
    return EffectiveUiTheme::Light;
  }
  if (p != QStringLiteral("system")) {
    return EffectiveUiTheme::Light;
  }
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
  if (qGuiApp == nullptr) {
    return EffectiveUiTheme::Light;
  }
  switch (QGuiApplication::styleHints()->colorScheme()) {
    case Qt::ColorScheme::Dark:
      return EffectiveUiTheme::Dark;
    case Qt::ColorScheme::Light:
      return EffectiveUiTheme::Light;
    case Qt::ColorScheme::Unknown:
    default:
      return EffectiveUiTheme::Light;
  }
#else
  return EffectiveUiTheme::Light;
#endif
}

QString UiTheme::mainWindowStyleSheet(EffectiveUiTheme theme) {
  if (theme == EffectiveUiTheme::Dark) {
    return QStringLiteral(
        "QMainWindow{background:#0f172a;}"
        "QTabWidget::pane{border:0;background:transparent;}"
        "QTabBar::tab{background:#1e293b;border:1px solid #334155;border-bottom:0;"
        "padding:10px 14px;margin-right:6px;border-top-left-radius:10px;border-top-right-radius:10px;"
        "color:#cbd5e1;font-weight:700;}"
        "QTabBar::tab:selected{background:#1e293b;color:#60a5fa;border-color:#3b82f6;}"
        "QTabBar::tab:!selected{background:#0f172a;}"
        "QWidget#TopBar{background:#1e293b;border:1px solid #334155;border-radius:12px;}"
        "QWidget#SideBar{background:#1e293b;border:1px solid #334155;border-radius:12px;}"
        "QLineEdit{background:#0f172a;border:1px solid #475569;border-radius:10px;"
        "padding:8px 12px;color:#e2e8f0;}"
        "QLineEdit:focus{border:1px solid #60a5fa;}"
        "QPushButton{background:#1e3a5f;color:#e2e8f0;border:1px solid #334155;border-radius:10px;"
        "padding:8px 14px;font-weight:600;}"
        "QPushButton:hover{background:#2563eb;color:#ffffff;}"
        "QPushButton#PrimaryButton{background:#2563eb;color:#ffffff;border:1px solid #2563eb;}"
        "QPushButton#PrimaryButton:hover{background:#3b82f6;}"
        "QPushButton#FilterButton{background:#1e293b;color:#cbd5e1;border:1px solid "
        "#475569;border-radius:14px;}"
        "QPushButton#FilterButton:checked{background:#2563eb;color:#ffffff;border:1px solid #2563eb;}"
        "QPushButton#FilterMoreButton{background:#1e293b;color:#cbd5e1;border:1px solid "
        "#475569;border-radius:14px;padding:8px 10px;font-weight:600;}"
        "QPushButton#FilterMoreButton:checked{background:#334155;color:#e2e8f0;border:1px solid "
        "#475569;}"
        "QWidget#StatCard{background:#1e293b;border:1px solid #334155;border-radius:12px;}"
        "QWidget#BottomStatusBar{background:#1e293b;border:1px solid #334155;border-radius:12px;}"
        "QLabel#BottomStatusLabel{color:#cbd5e1;font-size:12px;font-weight:600;}"
        "QLabel#StatTitle{color:#94a3b8;font-size:12px;}"
        "QLabel#FilterSideTitle{color:#e2e8f0;font-size:14px;font-weight:700;}"
        "QLabel#StatValue{color:#f1f5f9;font-size:22px;font-weight:700;}"
        "QTableWidget{background:#1e293b;border:1px solid #334155;border-radius:12px;"
        "gridline-color:#334155;selection-background-color:#2563eb;selection-color:#ffffff;"
        "color:#e2e8f0;alternate-background-color:#172033;}"
        "QTableWidget::item:selected{color:#ffffff;}"
        "QHeaderView::section{background:#0f172a;color:#cbd5e1;border:none;border-bottom:1px solid "
        "#334155;"
        "padding:8px;font-weight:600;}"
        "QComboBox{background:#0f172a;color:#e2e8f0;border:1px solid #475569;border-radius:8px;"
        "padding:4px 8px;}"
        "QComboBox QAbstractItemView{background:#1e293b;color:#e2e8f0;border:1px solid #475569;}");
  }
  return QStringLiteral(
      "QMainWindow{background:#f5f7fb;}"
      "QTabWidget::pane{border:0;background:transparent;}"
      "QTabBar::tab{background:#ffffff;border:1px solid #e7ebf3;border-bottom:0;"
      "padding:10px 14px;margin-right:6px;border-top-left-radius:10px;border-top-right-radius:10px;"
      "color:#41516d;font-weight:700;}"
      "QTabBar::tab:selected{background:#ffffff;color:#1677ff;border-color:#d6e3ff;}"
      "QTabBar::tab:!selected{background:#f8fbff;}"
      "QWidget#TopBar{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QWidget#SideBar{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QLineEdit{background:#f9fbff;border:1px solid #d8e2f0;border-radius:10px;"
      "padding:8px 12px;color:#1f2937;}"
      "QLineEdit:focus{border:1px solid #409eff;}"
      "QPushButton{background:#eef3ff;color:#2b3a55;border:1px solid #d6e3ff;border-radius:10px;"
      "padding:8px 14px;font-weight:600;}"
      "QPushButton:hover{background:#e4edff;}"
      "QPushButton#PrimaryButton{background:#1677ff;color:#ffffff;border:1px solid #1677ff;}"
      "QPushButton#PrimaryButton:hover{background:#3d8bff;}"
      "QPushButton#FilterButton{background:#f5f8ff;color:#41516d;border:1px solid "
      "#d9e5ff;border-radius:14px;}"
      "QPushButton#FilterButton:checked{background:#1677ff;color:#ffffff;border:1px solid #1677ff;}"
      "QPushButton#FilterMoreButton{background:#f5f8ff;color:#41516d;border:1px solid "
      "#d9e5ff;border-radius:14px;padding:8px 10px;font-weight:600;}"
      "QPushButton#FilterMoreButton:checked{background:#e8eefc;color:#1f2937;border:1px solid "
      "#b6c8ee;}"
      "QWidget#StatCard{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QWidget#BottomStatusBar{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QLabel#BottomStatusLabel{color:#41516d;font-size:12px;font-weight:600;}"
      "QLabel#StatTitle{color:#6b7280;font-size:12px;}"
      "QLabel#FilterSideTitle{color:#1f2937;font-size:14px;font-weight:700;}"
      "QLabel#StatValue{color:#1f2937;font-size:22px;font-weight:700;}"
      "QTableWidget{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;"
      "gridline-color:#eff3f9;selection-background-color:#2f6fed;selection-color:#ffffff;}"
      "QTableWidget::item:selected{color:#ffffff;}"
      "QHeaderView::section{background:#f8fbff;color:#50607a;border:none;border-bottom:1px solid "
      "#e7ebf3;"
      "padding:8px;font-weight:600;}");
}

QString UiTheme::settingsDialogStyleSheet(EffectiveUiTheme theme) {
  if (theme == EffectiveUiTheme::Dark) {
    return QStringLiteral(
        "QDialog#settingsDialog {"
        "  background: #0f172a;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid #334155;"
        "  border-radius: 10px;"
        "  background: #1e293b;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background: #1e293b;"
        "  color: #cbd5e1;"
        "  border: 1px solid #334155;"
        "  border-bottom: none;"
        "  border-top-left-radius: 8px;"
        "  border-top-right-radius: 8px;"
        "  padding: 9px 16px;"
        "  margin-right: 6px;"
        "  font-weight: 500;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1e293b;"
        "  color: #60a5fa;"
        "}"
        "QGroupBox {"
        "  border: 1px solid #334155;"
        "  border-radius: 10px;"
        "  margin-top: 10px;"
        "  padding: 12px;"
        "  background: #1e293b;"
        "  font-weight: 600;"
        "  color: #e2e8f0;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 4px;"
        "  color: #f1f5f9;"
        "}"
        "QFormLayout {"
        "  spacing: 10px;"
        "}"
        "QLineEdit, QPlainTextEdit {"
        "  border: 1px solid #475569;"
        "  border-radius: 8px;"
        "  background: #0f172a;"
        "  color: #e2e8f0;"
        "  padding: 4px 8px;"
        "  min-height: 18px;"
        "}"
        "QLineEdit:focus, QPlainTextEdit:focus {"
        "  border: 1px solid #60a5fa;"
        "}"
        "QComboBox, QSpinBox, QDoubleSpinBox {"
        "  color: #e2e8f0;"
        "  background: #0f172a;"
        "  border: 1px solid #475569;"
        "  border-radius: 8px;"
        "  padding: 2px 8px;"
        "}"
        "QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button, "
        "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
        "  background: transparent;"
        "}"
        "QComboBox QAbstractItemView {"
        "  border: 1px solid #475569;"
        "  background: #1e293b;"
        "  color: #e2e8f0;"
        "  padding: 2px;"
        "  outline: 0;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  min-height: 22px;"
        "  padding: 2px 8px;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "  background: #2563eb;"
        "  color: #ffffff;"
        "}"
        "QTabBar QToolButton {"
        "  border: 1px solid #475569;"
        "  background: #0f172a;"
        "  color: #e2e8f0;"
        "  border-radius: 6px;"
        "  padding: 2px;"
        "  min-width: 18px;"
        "}"
        "QPushButton {"
        "  border: 1px solid #475569;"
        "  border-radius: 8px;"
        "  background: #1e293b;"
        "  color: #e2e8f0;"
        "  padding: 7px 14px;"
        "}"
        "QPushButton:hover {"
        "  background: #334155;"
        "}"
        "QDialogButtonBox QPushButton {"
        "  min-width: 92px;"
        "}"
        "QDialogButtonBox QPushButton[text=\"确定\"] {"
        "  background: #2563eb;"
        "  color: #ffffff;"
        "  border-color: #2563eb;"
        "}"
        "QDialogButtonBox QPushButton[text=\"确定\"]:hover {"
        "  background: #3b82f6;"
        "}"
        "QLabel {"
        "  color: #cbd5e1;"
        "  qproperty-wordWrap: true;"
        "}"
        "QLabel[class=\"sectionHint\"] {"
        "  color: #94a3b8;"
        "  padding-left: 2px;"
        "}"
        "QCheckBox {"
        "  spacing: 7px;"
        "  color: #e2e8f0;"
        "}");
  }
  return QStringLiteral(
      "QDialog#settingsDialog {"
      "  background: #f5f7fb;"
      "}"
      "QTabWidget::pane {"
      "  border: 1px solid #dfe3eb;"
      "  border-radius: 10px;"
      "  background: #ffffff;"
      "  top: -1px;"
      "}"
      "QTabBar::tab {"
      "  background: #eef1f7;"
      "  color: #3a4150;"
      "  border: 1px solid #dfe3eb;"
      "  border-bottom: none;"
      "  border-top-left-radius: 8px;"
      "  border-top-right-radius: 8px;"
      "  padding: 9px 16px;"
      "  margin-right: 6px;"
      "  font-weight: 500;"
      "}"
      "QTabBar::tab:selected {"
      "  background: #ffffff;"
      "  color: #1f6feb;"
      "}"
      "QGroupBox {"
      "  border: 1px solid #e4e8f0;"
      "  border-radius: 10px;"
      "  margin-top: 10px;"
      "  padding: 12px;"
      "  background: #ffffff;"
      "  font-weight: 600;"
      "}"
      "QGroupBox::title {"
      "  subcontrol-origin: margin;"
      "  left: 12px;"
      "  padding: 0 4px;"
      "  color: #2f3542;"
      "}"
      "QFormLayout {"
      "  spacing: 10px;"
      "}"
      "QLineEdit, QPlainTextEdit {"
      "  border: 1px solid #d0d7e2;"
      "  border-radius: 8px;"
      "  background: #ffffff;"
      "  padding: 4px 8px;"
      "  min-height: 18px;"
      "}"
      "QLineEdit:focus, QPlainTextEdit:focus {"
      "  border: 1px solid #1f6feb;"
      "}"
      "QComboBox, QSpinBox, QDoubleSpinBox {"
      "  color: #1f2937;"
      "  background: #ffffff;"
      "}"
      "QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button, "
      "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {"
      "  background: transparent;"
      "}"
      "QComboBox QAbstractItemView {"
      "  border: 1px solid #d0d7e2;"
      "  background: #ffffff;"
      "  color: #1f2937;"
      "  padding: 2px;"
      "  outline: 0;"
      "}"
      "QComboBox QAbstractItemView::item {"
      "  min-height: 22px;"
      "  padding: 2px 8px;"
      "}"
      "QComboBox QAbstractItemView::item:selected {"
      "  background: #1f6feb;"
      "  color: #ffffff;"
      "}"
      "QTabBar QToolButton {"
      "  border: 1px solid #d0d7e2;"
      "  background: #ffffff;"
      "  color: #1f2937;"
      "  border-radius: 6px;"
      "  padding: 2px;"
      "  min-width: 18px;"
      "}"
      "QPushButton {"
      "  border: 1px solid #d0d7e2;"
      "  border-radius: 8px;"
      "  background: #ffffff;"
      "  padding: 7px 14px;"
      "}"
      "QPushButton:hover {"
      "  background: #f3f6fb;"
      "}"
      "QDialogButtonBox QPushButton {"
      "  min-width: 92px;"
      "}"
      "QDialogButtonBox QPushButton[text=\"确定\"] {"
      "  background: #1f6feb;"
      "  color: #ffffff;"
      "  border-color: #1f6feb;"
      "}"
      "QDialogButtonBox QPushButton[text=\"确定\"]:hover {"
      "  background: #1760cf;"
      "}"
      "QLabel {"
      "  color: #4c5566;"
      "  qproperty-wordWrap: true;"
      "}"
      "QLabel[class=\"sectionHint\"] {"
      "  color: #6a7280;"
      "  padding-left: 2px;"
      "}"
      "QCheckBox {"
      "  spacing: 7px;"
      "}");
}

QString UiTheme::auxiliaryDialogStyleSheet(const QString& dialogObjectName, EffectiveUiTheme theme) {
  const QString esc = dialogObjectName.trimmed();
  if (esc.isEmpty()) {
    return {};
  }
  const QString sel = QStringLiteral("QDialog#%1").arg(esc);
  if (theme == EffectiveUiTheme::Dark) {
    QString s;
    s += sel + QStringLiteral("{background:#0f172a;}");
    s += sel + QStringLiteral(" QGroupBox{border:1px solid #334155;border-radius:10px;margin-top:10px;"
                               "padding:12px;background:#1e293b;font-weight:600;color:#e2e8f0;}");
    s += sel + QStringLiteral(" QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 4px;"
                               "color:#f1f5f9;}");
    s += sel + QStringLiteral(" QLabel{color:#cbd5e1;}");
    s += sel + QStringLiteral(" QLabel#DialogTitleLabel{font-size:18px;font-weight:800;color:#f1f5f9;}");
    s += sel + QStringLiteral(" QLineEdit,") + sel + QStringLiteral(" QComboBox,") + sel +
        QStringLiteral(" QSpinBox{border:1px solid #475569;border-radius:8px;background:#0f172a;"
                       "color:#e2e8f0;padding:4px 8px;}");
    s += sel + QStringLiteral(" QTreeWidget{background:#0f172a;color:#e2e8f0;border:1px solid #475569;"
                               "border-radius:8px;}");
    s += sel + QStringLiteral(" QTreeWidget::item:selected{background:#2563eb;color:#ffffff;}");
    s += sel + QStringLiteral(" QHeaderView::section{background:#1e293b;color:#cbd5e1;border:none;"
                               "padding:6px;font-weight:600;}");
    s += sel + QStringLiteral(" QPushButton{background:#1e293b;color:#e2e8f0;border:1px solid #475569;"
                               "border-radius:8px;padding:6px 14px;}");
    s += sel + QStringLiteral(" QPushButton:hover{background:#334155;}");
    s += sel + QStringLiteral(" QSplitter::handle{background:#334155;}");
    s += sel + QStringLiteral(" QCheckBox{color:#e2e8f0;spacing:8px;}");
    s += sel + QStringLiteral(" QDialogButtonBox QPushButton[text=\"确定\"]{background:#2563eb;"
                               "color:#ffffff;border:1px solid #2563eb;border-radius:8px;}");
    s += sel + QStringLiteral(" QDialogButtonBox QPushButton[text=\"确定\"]:hover{background:#3b82f6;}");
    s += sel + QStringLiteral(" QPlainTextEdit{background:#0f172a;color:#e2e8f0;border:1px solid #475569;"
                               "border-radius:8px;font-family:monospace;padding:8px;}");
    s += sel + QStringLiteral(" QComboBox QAbstractItemView{border:1px solid #475569;background:#1e293b;"
                               "color:#e2e8f0;}");
    return s;
  }
  QString s;
  s += sel + QStringLiteral("{background:#f5f7fb;}");
  s += sel + QStringLiteral(" QGroupBox{border:1px solid #e4e8f0;border-radius:10px;margin-top:10px;"
                             "padding:12px;background:#ffffff;font-weight:600;color:#1f2937;}");
  s += sel + QStringLiteral(" QGroupBox::title{subcontrol-origin:margin;left:12px;padding:0 4px;"
                             "color:#2f3542;}");
  s += sel + QStringLiteral(" QLabel{color:#4c5566;}");
  s += sel + QStringLiteral(" QLabel#DialogTitleLabel{font-size:18px;font-weight:800;color:#1f2d3d;}");
  s += sel + QStringLiteral(" QLineEdit,") + sel + QStringLiteral(" QComboBox,") + sel +
      QStringLiteral(" QSpinBox{border:1px solid #d0d7e2;border-radius:8px;background:#ffffff;"
                     "color:#1f2937;padding:4px 8px;}");
  s += sel + QStringLiteral(" QTreeWidget{background:#ffffff;color:#1f2937;border:1px solid #d0d7e2;"
                             "border-radius:8px;}");
  s += sel + QStringLiteral(" QTreeWidget::item:selected{background:#1f6feb;color:#ffffff;}");
  s += sel + QStringLiteral(" QHeaderView::section{background:#f8fbff;color:#50607a;border:none;"
                             "padding:6px;font-weight:600;}");
  s += sel + QStringLiteral(" QPushButton{background:#ffffff;color:#1f2937;border:1px solid #d0d7e2;"
                             "border-radius:8px;padding:6px 14px;}");
  s += sel + QStringLiteral(" QPushButton:hover{background:#f3f6fb;}");
  s += sel + QStringLiteral(" QSplitter::handle{background:#dfe3eb;}");
  s += sel + QStringLiteral(" QCheckBox{color:#1f2937;spacing:8px;}");
  s += sel + QStringLiteral(" QDialogButtonBox QPushButton[text=\"确定\"]{background:#1f6feb;"
                             "color:#ffffff;border:1px solid #1f6feb;border-radius:8px;}");
  s += sel + QStringLiteral(" QDialogButtonBox QPushButton[text=\"确定\"]:hover{background:#1760cf;}");
  s += sel + QStringLiteral(" QPlainTextEdit{background:#ffffff;color:#1f2937;border:1px solid #d0d7e2;"
                             "border-radius:8px;font-family:monospace;padding:8px;}");
  s += sel + QStringLiteral(" QComboBox QAbstractItemView{border:1px solid #d0d7e2;background:#ffffff;"
                             "color:#1f2937;}");
  return s;
}

QVariantMap UiTheme::speedChartThemeTokens(EffectiveUiTheme theme) {
  QVariantMap m;
  if (theme == EffectiveUiTheme::Dark) {
    m.insert(QStringLiteral("colorSurface"), QStringLiteral("#1e293b"));
    m.insert(QStringLiteral("colorTopBarBg"), QStringLiteral("#0f172a"));
    m.insert(QStringLiteral("colorTopBarBorder"), QStringLiteral("#334155"));
    m.insert(QStringLiteral("colorTitleText"), QStringLiteral("#f1f5f9"));
    m.insert(QStringLiteral("colorMutedText"), QStringLiteral("#94a3b8"));
    m.insert(QStringLiteral("colorMetricDlBg"), QStringLiteral("#172554"));
    m.insert(QStringLiteral("colorMetricDlBorder"), QStringLiteral("#334155"));
    m.insert(QStringLiteral("colorMetricDlLabel"), QStringLiteral("#93c5fd"));
    m.insert(QStringLiteral("colorMetricDlValue"), QStringLiteral("#f1f5f9"));
    m.insert(QStringLiteral("colorMetricUlBg"), QStringLiteral("#431407"));
    m.insert(QStringLiteral("colorMetricUlBorder"), QStringLiteral("#7c2d12"));
    m.insert(QStringLiteral("colorMetricUlLabel"), QStringLiteral("#fdba74"));
    m.insert(QStringLiteral("colorMetricUlValue"), QStringLiteral("#f1f5f9"));
    m.insert(QStringLiteral("colorMetricPeakBg"), QStringLiteral("#2e1065"));
    m.insert(QStringLiteral("colorMetricPeakBorder"), QStringLiteral("#5b21b6"));
    m.insert(QStringLiteral("colorMetricPeakLabel"), QStringLiteral("#c4b5fd"));
    m.insert(QStringLiteral("colorMetricPeakValue"), QStringLiteral("#f1f5f9"));
    m.insert(QStringLiteral("colorControlBg"), QStringLiteral("#0f172a"));
    m.insert(QStringLiteral("colorControlBorder"), QStringLiteral("#475569"));
    m.insert(QStringLiteral("colorControlText"), QStringLiteral("#cbd5e1"));
    m.insert(QStringLiteral("colorSeriesPanelBg"), QStringLiteral("#0f172acc"));
    m.insert(QStringLiteral("colorSeriesPanelBorder"), QStringLiteral("#334155"));
    m.insert(QStringLiteral("colorSeriesLabel"), QStringLiteral("#cbd5e1"));
    m.insert(QStringLiteral("colorCheckboxBorder"), QStringLiteral("#64748b"));
    m.insert(QStringLiteral("colorCheckboxUnchecked"), QStringLiteral("#1e293b"));
    m.insert(QStringLiteral("colorGridMajor"), QStringLiteral("#334155"));
    m.insert(QStringLiteral("colorAxisLine"), QStringLiteral("#64748b"));
    m.insert(QStringLiteral("colorAxisBase"), QStringLiteral("#94a3b8"));
    m.insert(QStringLiteral("colorTickLabel"), QStringLiteral("#94a3b8"));
    m.insert(QStringLiteral("colorLegendText"), QStringLiteral("#cbd5e1"));
    m.insert(QStringLiteral("colorAreaDl"), QStringLiteral("rgba(56, 189, 248, 0.18)"));
    m.insert(QStringLiteral("colorAreaUl"), QStringLiteral("rgba(251, 146, 60, 0.14)"));
    return m;
  }
  m.insert(QStringLiteral("colorSurface"), QStringLiteral("#ffffff"));
  m.insert(QStringLiteral("colorTopBarBg"), QStringLiteral("#f8fafc"));
  m.insert(QStringLiteral("colorTopBarBorder"), QStringLiteral("#e2e8f0"));
  m.insert(QStringLiteral("colorTitleText"), QStringLiteral("#0f172a"));
  m.insert(QStringLiteral("colorMutedText"), QStringLiteral("#64748b"));
  m.insert(QStringLiteral("colorMetricDlBg"), QStringLiteral("#eff6ff"));
  m.insert(QStringLiteral("colorMetricDlBorder"), QStringLiteral("#bfdbfe"));
  m.insert(QStringLiteral("colorMetricDlLabel"), QStringLiteral("#0369a1"));
  m.insert(QStringLiteral("colorMetricDlValue"), QStringLiteral("#0f172a"));
  m.insert(QStringLiteral("colorMetricUlBg"), QStringLiteral("#fff7ed"));
  m.insert(QStringLiteral("colorMetricUlBorder"), QStringLiteral("#fed7aa"));
  m.insert(QStringLiteral("colorMetricUlLabel"), QStringLiteral("#b45309"));
  m.insert(QStringLiteral("colorMetricUlValue"), QStringLiteral("#0f172a"));
  m.insert(QStringLiteral("colorMetricPeakBg"), QStringLiteral("#f5f3ff"));
  m.insert(QStringLiteral("colorMetricPeakBorder"), QStringLiteral("#ddd6fe"));
  m.insert(QStringLiteral("colorMetricPeakLabel"), QStringLiteral("#6d28d9"));
  m.insert(QStringLiteral("colorMetricPeakValue"), QStringLiteral("#0f172a"));
  m.insert(QStringLiteral("colorControlBg"), QStringLiteral("#ffffff"));
  m.insert(QStringLiteral("colorControlBorder"), QStringLiteral("#cbd5e1"));
  m.insert(QStringLiteral("colorControlText"), QStringLiteral("#334155"));
  m.insert(QStringLiteral("colorSeriesPanelBg"), QStringLiteral("#ffffffee"));
  m.insert(QStringLiteral("colorSeriesPanelBorder"), QStringLiteral("#e2e8f0"));
  m.insert(QStringLiteral("colorSeriesLabel"), QStringLiteral("#334155"));
  m.insert(QStringLiteral("colorCheckboxBorder"), QStringLiteral("#94a3b8"));
  m.insert(QStringLiteral("colorCheckboxUnchecked"), QStringLiteral("#ffffff"));
  m.insert(QStringLiteral("colorGridMajor"), QStringLiteral("#e2e8f0"));
  m.insert(QStringLiteral("colorAxisLine"), QStringLiteral("#94a3b8"));
  m.insert(QStringLiteral("colorAxisBase"), QStringLiteral("#64748b"));
  m.insert(QStringLiteral("colorTickLabel"), QStringLiteral("#64748b"));
  m.insert(QStringLiteral("colorLegendText"), QStringLiteral("#334155"));
  m.insert(QStringLiteral("colorAreaDl"), QStringLiteral("rgba(56, 189, 248, 0.14)"));
  m.insert(QStringLiteral("colorAreaUl"), QStringLiteral("rgba(251, 146, 60, 0.10)"));
  return m;
}

QString UiTheme::detailTabBarStyleSheet(EffectiveUiTheme theme) {
  if (theme == EffectiveUiTheme::Dark) {
    return QStringLiteral(
        "QWidget#DetailTabBar{background:#0f172a;border-top:1px solid #334155;}"
        "QWidget#DetailTabBar QPushButton#DetailTabButton{border:none;border-radius:6px;"
        "padding:4px 10px;font-size:12px;color:#94a3b8;background:transparent;}"
        "QWidget#DetailTabBar QPushButton#DetailTabButton:checked{background:#1e3a5f;color:#93c5fd;"
        "font-weight:700;}"
        "QWidget#DetailTabBar QPushButton#DetailTabButton:hover{background:#1e293b;color:#cbd5e1;}");
  }
  return QStringLiteral(
      "QWidget#DetailTabBar{background:#f8fafc;border-top:1px solid #e2e8f0;}"
      "QWidget#DetailTabBar QPushButton#DetailTabButton{border:none;border-radius:6px;"
      "padding:4px 10px;font-size:12px;color:#64748b;background:transparent;}"
      "QWidget#DetailTabBar QPushButton#DetailTabButton:checked{background:#e8eefc;color:#1e40af;"
      "font-weight:700;}"
      "QWidget#DetailTabBar QPushButton#DetailTabButton:hover{background:#f1f5f9;color:#334155;}");
}

QString UiTheme::trackerTreeStyleSheet(EffectiveUiTheme theme) {
  if (theme == EffectiveUiTheme::Dark) {
    return QStringLiteral(
        "QTreeWidget{border:1px solid #334155;border-radius:12px;background:#1e293b;color:#e2e8f0;"
        "alternate-background-color:#172033;outline:0;"
        "selection-background-color:#2563eb;selection-color:#ffffff;}"
        "QTreeWidget::item{padding:4px 8px;border:none;}"
        "QTreeWidget::item:selected,QTreeWidget::item:selected:active{"
        "background:#2563eb;color:#ffffff;}"
        "QHeaderView::section{background:#0f172a;color:#cbd5e1;border:none;border-bottom:1px solid "
        "#334155;padding:8px 10px;font-weight:600;}");
  }
  return QStringLiteral(
      "QTreeWidget{border:1px solid #e7ebf3;border-radius:12px;background:#ffffff;"
      "alternate-background-color:#f6f8fc;outline:0;"
      "selection-background-color:#2f6fed;selection-color:#ffffff;}"
      "QTreeWidget::item{padding:4px 8px;border:none;}"
      "QTreeWidget::item:selected,QTreeWidget::item:selected:active{"
      "background:#2f6fed;color:#ffffff;}"
      "QHeaderView::section{background:#f8fbff;color:#50607a;border:none;border-bottom:1px solid "
      "#e7ebf3;padding:8px 10px;font-weight:600;}");
}

}  // namespace pfd::ui
