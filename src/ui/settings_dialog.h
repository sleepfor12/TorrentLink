#ifndef PFD_UI_SETTINGS_DIALOG_H
#define PFD_UI_SETTINGS_DIALOG_H

#include <QtWidgets/QDialog>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QDoubleSpinBox;
class QComboBox;
class QSpinBox;
class QLabel;

class QPlainTextEdit;

namespace pfd::core {
struct AppSettings;
struct ControllerSettings;
}  // namespace pfd::core
namespace pfd::core::rss {
struct RssSettings;
}

namespace pfd::ui {

class SettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit SettingsDialog(QWidget* parent = nullptr);

  [[nodiscard]] pfd::core::AppSettings currentSettings() const;
  [[nodiscard]] pfd::core::rss::RssSettings currentRssSettings() const;
  [[nodiscard]] pfd::core::ControllerSettings currentControllerSettings() const;
  void setSettings(const pfd::core::AppSettings& s);
  void setRssSettings(const pfd::core::rss::RssSettings& s);
  void setControllerSettings(const pfd::core::ControllerSettings& s);

private:
  void buildLayout();
  void wireSignals();
  void syncDownloadCompleteActionAvailability();
  void browseDownloadDir();
  void browseLogDir();

  QLineEdit* downloadDirEdit_{nullptr};
  QPushButton* browseDownloadDirBtn_{nullptr};

  QCheckBox* seedUnlimitedCheck_{nullptr};
  QDoubleSpinBox* seedTargetRatioSpin_{nullptr};
  QSpinBox* seedMaxMinutesSpin_{nullptr};

  QCheckBox* fileLogEnabledCheck_{nullptr};
  QComboBox* logLevelBox_{nullptr};
  QCheckBox* logRotateEnabledCheck_{nullptr};
  QSpinBox* logRotateMaxSizeMiB_{nullptr};
  QSpinBox* logRotateBackups_{nullptr};
  QLineEdit* logDirEdit_{nullptr};
  QPushButton* browseLogDirBtn_{nullptr};

  QSpinBox* downloadLimitKiB_{nullptr};
  QSpinBox* uploadLimitKiB_{nullptr};
  QSpinBox* connectionsLimit_{nullptr};
  QSpinBox* perTorrentConnectionsLimit_{nullptr};
  QSpinBox* uploadSlotsLimit_{nullptr};
  QSpinBox* perTorrentUploadSlotsLimit_{nullptr};
  QSpinBox* listenPortSpin_{nullptr};
  QPushButton* randomListenPortBtn_{nullptr};
  QCheckBox* listenPortForwardingCheck_{nullptr};

  QSpinBox* activeDownloads_{nullptr};
  QSpinBox* activeSeeds_{nullptr};
  QSpinBox* activeLimit_{nullptr};
  QCheckBox* preferSeedsCheck_{nullptr};

  QCheckBox* enableDhtCheck_{nullptr};
  QCheckBox* enableUpnpCheck_{nullptr};
  QCheckBox* enableNatpmpCheck_{nullptr};
  QCheckBox* enableLsdCheck_{nullptr};
  QSpinBox* monitorPortSpin_{nullptr};
  QCheckBox* builtinTrackerEnabledCheck_{nullptr};
  QSpinBox* builtinTrackerPortSpin_{nullptr};
  QCheckBox* builtinTrackerPortForwardingCheck_{nullptr};
  QComboBox* encryptionModeBox_{nullptr};
  QCheckBox* ipFilterEnabledCheck_{nullptr};
  QLineEdit* ipFilterPathEdit_{nullptr};
  QCheckBox* proxyEnabledCheck_{nullptr};
  QComboBox* proxyTypeBox_{nullptr};
  QLineEdit* proxyHostEdit_{nullptr};
  QSpinBox* proxyPortSpin_{nullptr};
  QLineEdit* proxyUsernameEdit_{nullptr};
  QLineEdit* proxyPasswordEdit_{nullptr};

  QCheckBox* autoApplyTrackersCheck_{nullptr};
  QPlainTextEdit* defaultTrackersEdit_{nullptr};
  QSpinBox* magnetMaxInflightSpin_{nullptr};
  QCheckBox* bottomStatusEnabledCheck_{nullptr};
  QSpinBox* bottomStatusRefreshMsSpin_{nullptr};
  QSpinBox* taskAutoSaveMsSpin_{nullptr};
  QCheckBox* restoreStartPausedCheck_{nullptr};

  QComboBox* closeBehaviorBox_{nullptr};
  QCheckBox* startMinimizedCheck_{nullptr};
  QComboBox* timedActionBox_{nullptr};
  QSpinBox* timedActionDelaySpin_{nullptr};
  QComboBox* downloadCompleteActionBox_{nullptr};
  QLabel* downloadCompleteActionHint_{nullptr};

  QCheckBox* rssGlobalAutoDownloadCheck_{nullptr};
  QSpinBox* rssRefreshIntervalSpin_{nullptr};
  QSpinBox* rssMaxAutoDownloadsSpin_{nullptr};
  QSpinBox* rssHistoryMaxItemsSpin_{nullptr};
  QSpinBox* rssHistoryMaxAgeSpin_{nullptr};
  QLineEdit* rssPlayerCommandEdit_{nullptr};
  QLineEdit* httpUserAgentEdit_{nullptr};
  QLineEdit* httpAcceptLanguageEdit_{nullptr};
  QPlainTextEdit* httpCookieHeaderEdit_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_SETTINGS_DIALOG_H
