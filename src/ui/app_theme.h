#ifndef PFD_UI_APP_THEME_H
#define PFD_UI_APP_THEME_H

#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace pfd::ui {

enum class EffectiveUiTheme { Light, Dark };

class UiTheme {
public:
  /// @param storedPreference 已持久化的偏好："light" | "dark" | "system"
  [[nodiscard]] static EffectiveUiTheme resolveEffectiveTheme(const QString& storedPreference);

  [[nodiscard]] static QString mainWindowStyleSheet(EffectiveUiTheme theme);

  [[nodiscard]] static QString settingsDialogStyleSheet(EffectiveUiTheme theme);

  /// 添加任务、日志中心等辅助对话框（objectName 与参数一致，例如 addTorrentDialog）
  [[nodiscard]] static QString auxiliaryDialogStyleSheet(const QString& dialogObjectName,
                                                         EffectiveUiTheme theme);

  /// SpeedChart.qml 根节点上的字符串属性（由 C++ setProperty 注入）
  [[nodiscard]] static QVariantMap speedChartThemeTokens(EffectiveUiTheme theme);

  /// 任务详情「Tracker」标签页内 QTreeWidget + 表头（与主窗口表格风格一致）
  [[nodiscard]] static QString trackerTreeStyleSheet(EffectiveUiTheme theme);

  /// 任务详情底栏（#DetailTabBar + #DetailTabButton）
  [[nodiscard]] static QString detailTabBarStyleSheet(EffectiveUiTheme theme);
};

}  // namespace pfd::ui

#endif  // PFD_UI_APP_THEME_H
