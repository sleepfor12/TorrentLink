#include "ui/settings_dialog.h"

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>

#include "core/app_settings.h"
#include "core/config_service.h"
#include "core/rss/rss_types.h"

namespace pfd::ui {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("首选项"));
  setModal(true);
  resize(920, 680);
  setMinimumSize(860, 620);
  setObjectName(QStringLiteral("settingsDialog"));
  setStyleSheet(QStringLiteral("QDialog#settingsDialog {"
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
                               "}"));
  buildLayout();
  wireSignals();
}

pfd::core::AppSettings SettingsDialog::currentSettings() const {
  pfd::core::AppSettings s;
  s.default_download_dir =
      downloadDirEdit_ != nullptr ? downloadDirEdit_->text().trimmed() : QString();
  s.seed_unlimited = seedUnlimitedCheck_ != nullptr && seedUnlimitedCheck_->isChecked();
  s.seed_target_ratio = seedTargetRatioSpin_ != nullptr ? seedTargetRatioSpin_->value() : 1.00;
  s.seed_max_minutes = seedMaxMinutesSpin_ != nullptr ? seedMaxMinutesSpin_->value() : 0;
  s.file_log_enabled = fileLogEnabledCheck_ != nullptr && fileLogEnabledCheck_->isChecked();
  s.log_level =
      logLevelBox_ != nullptr ? logLevelBox_->currentData().toString() : QStringLiteral("info");
  const bool rotateEnabled =
      logRotateEnabledCheck_ != nullptr && logRotateEnabledCheck_->isChecked();
  const qint64 rotateMiB = logRotateMaxSizeMiB_ != nullptr ? logRotateMaxSizeMiB_->value() : 0;
  s.log_rotate_max_file_size_bytes = rotateEnabled ? (rotateMiB * 1024LL * 1024LL) : 0;
  s.log_rotate_max_backup_files = logRotateBackups_ != nullptr ? logRotateBackups_->value() : 3;
  s.log_dir = logDirEdit_ != nullptr ? logDirEdit_->text().trimmed() : QString();
  s.download_rate_limit_kib = downloadLimitKiB_ != nullptr ? downloadLimitKiB_->value() : 0;
  s.upload_rate_limit_kib = uploadLimitKiB_ != nullptr ? uploadLimitKiB_->value() : 0;
  s.connections_limit = connectionsLimit_ != nullptr ? connectionsLimit_->value() : 200;
  s.per_torrent_connections_limit =
      perTorrentConnectionsLimit_ != nullptr ? perTorrentConnectionsLimit_->value() : 0;
  s.upload_slots_limit = uploadSlotsLimit_ != nullptr ? uploadSlotsLimit_->value() : 0;
  s.per_torrent_upload_slots_limit =
      perTorrentUploadSlotsLimit_ != nullptr ? perTorrentUploadSlotsLimit_->value() : 0;
  s.listen_port = listenPortSpin_ != nullptr ? listenPortSpin_->value() : 0;
  s.listen_port_forwarding =
      listenPortForwardingCheck_ != nullptr && listenPortForwardingCheck_->isChecked();
  s.active_downloads = activeDownloads_ != nullptr ? activeDownloads_->value() : 0;
  s.active_seeds = activeSeeds_ != nullptr ? activeSeeds_->value() : 0;
  s.active_limit = activeLimit_ != nullptr ? activeLimit_->value() : 0;
  s.auto_manage_prefer_seeds = preferSeedsCheck_ != nullptr && preferSeedsCheck_->isChecked();
  s.enable_dht = enableDhtCheck_ != nullptr && enableDhtCheck_->isChecked();
  s.enable_upnp = enableUpnpCheck_ != nullptr && enableUpnpCheck_->isChecked();
  s.enable_natpmp = enableNatpmpCheck_ != nullptr && enableNatpmpCheck_->isChecked();
  s.enable_lsd = enableLsdCheck_ != nullptr && enableLsdCheck_->isChecked();
  s.monitor_port = monitorPortSpin_ != nullptr ? monitorPortSpin_->value() : 0;
  s.builtin_tracker_enabled =
      builtinTrackerEnabledCheck_ != nullptr && builtinTrackerEnabledCheck_->isChecked();
  s.builtin_tracker_port =
      builtinTrackerPortSpin_ != nullptr ? builtinTrackerPortSpin_->value() : 0;
  s.builtin_tracker_port_forwarding = builtinTrackerPortForwardingCheck_ != nullptr &&
                                      builtinTrackerPortForwardingCheck_->isChecked();
  s.encryption_mode = encryptionModeBox_ != nullptr ? encryptionModeBox_->currentData().toString()
                                                    : QStringLiteral("enabled");
  s.ip_filter_enabled = ipFilterEnabledCheck_ != nullptr && ipFilterEnabledCheck_->isChecked();
  s.ip_filter_path = ipFilterPathEdit_ != nullptr ? ipFilterPathEdit_->text().trimmed() : QString();
  s.proxy_enabled = proxyEnabledCheck_ != nullptr && proxyEnabledCheck_->isChecked();
  s.proxy_type =
      proxyTypeBox_ != nullptr ? proxyTypeBox_->currentData().toString() : QStringLiteral("socks5");
  s.proxy_host = proxyHostEdit_ != nullptr ? proxyHostEdit_->text().trimmed() : QString();
  s.proxy_port = proxyPortSpin_ != nullptr ? proxyPortSpin_->value() : 1080;
  s.proxy_username =
      proxyUsernameEdit_ != nullptr ? proxyUsernameEdit_->text().trimmed() : QString();
  s.proxy_password = proxyPasswordEdit_ != nullptr ? proxyPasswordEdit_->text() : QString();
  s.close_behavior = closeBehaviorBox_ != nullptr ? closeBehaviorBox_->currentData().toString()
                                                  : QStringLiteral("minimize");
  s.start_minimized = startMinimizedCheck_ != nullptr && startMinimizedCheck_->isChecked();
  s.timed_action = timedActionBox_ != nullptr ? timedActionBox_->currentData().toString()
                                              : QStringLiteral("none");
  s.timed_action_delay_minutes =
      timedActionDelaySpin_ != nullptr ? timedActionDelaySpin_->value() : 0;
  return s;
}

pfd::core::rss::RssSettings SettingsDialog::currentRssSettings() const {
  pfd::core::rss::RssSettings s;
  s.global_auto_download =
      rssGlobalAutoDownloadCheck_ != nullptr && rssGlobalAutoDownloadCheck_->isChecked();
  s.refresh_interval_minutes =
      rssRefreshIntervalSpin_ != nullptr ? rssRefreshIntervalSpin_->value() : 30;
  s.max_auto_downloads_per_refresh =
      rssMaxAutoDownloadsSpin_ != nullptr ? rssMaxAutoDownloadsSpin_->value() : 10;
  s.history_max_items =
      rssHistoryMaxItemsSpin_ != nullptr ? rssHistoryMaxItemsSpin_->value() : 5000;
  s.history_max_age_days = rssHistoryMaxAgeSpin_ != nullptr ? rssHistoryMaxAgeSpin_->value() : 90;
  s.external_player_command =
      rssPlayerCommandEdit_ != nullptr ? rssPlayerCommandEdit_->text().trimmed() : QString();
  return s;
}

pfd::core::ControllerSettings SettingsDialog::currentControllerSettings() const {
  pfd::core::ControllerSettings s;
  s.auto_apply_default_trackers =
      autoApplyTrackersCheck_ != nullptr && autoApplyTrackersCheck_->isChecked();
  if (defaultTrackersEdit_ != nullptr) {
    s.default_trackers = defaultTrackersEdit_->toPlainText().split('\n', Qt::SkipEmptyParts);
  }
  s.magnet_max_inflight = magnetMaxInflightSpin_ != nullptr ? magnetMaxInflightSpin_->value() : 8;
  s.bottom_status_enabled =
      bottomStatusEnabledCheck_ != nullptr && bottomStatusEnabledCheck_->isChecked();
  s.bottom_status_refresh_ms =
      bottomStatusRefreshMsSpin_ != nullptr ? bottomStatusRefreshMsSpin_->value() : 1000;
  s.task_autosave_ms = taskAutoSaveMsSpin_ != nullptr ? taskAutoSaveMsSpin_->value() : 5000;
  s.restore_start_paused =
      restoreStartPausedCheck_ != nullptr && restoreStartPausedCheck_->isChecked();
  return s;
}

void SettingsDialog::setControllerSettings(const pfd::core::ControllerSettings& s) {
  if (autoApplyTrackersCheck_ != nullptr) {
    autoApplyTrackersCheck_->setChecked(s.auto_apply_default_trackers);
  }
  if (defaultTrackersEdit_ != nullptr) {
    defaultTrackersEdit_->setPlainText(s.default_trackers.join('\n'));
    defaultTrackersEdit_->setEnabled(s.auto_apply_default_trackers);
  }
  if (magnetMaxInflightSpin_ != nullptr) {
    magnetMaxInflightSpin_->setValue(std::clamp(s.magnet_max_inflight, 5, 10));
  }
  if (bottomStatusEnabledCheck_ != nullptr) {
    bottomStatusEnabledCheck_->setChecked(s.bottom_status_enabled);
  }
  if (bottomStatusRefreshMsSpin_ != nullptr) {
    bottomStatusRefreshMsSpin_->setValue(std::clamp(s.bottom_status_refresh_ms, 500, 5000));
    bottomStatusRefreshMsSpin_->setEnabled(s.bottom_status_enabled);
  }
  if (taskAutoSaveMsSpin_ != nullptr) {
    taskAutoSaveMsSpin_->setValue(std::clamp(s.task_autosave_ms, 2000, 60000));
  }
  if (restoreStartPausedCheck_ != nullptr) {
    restoreStartPausedCheck_->setChecked(s.restore_start_paused);
  }
}

void SettingsDialog::setSettings(const pfd::core::AppSettings& s) {
  if (downloadDirEdit_ != nullptr) {
    downloadDirEdit_->setText(s.default_download_dir);
  }
  if (seedUnlimitedCheck_ != nullptr) {
    seedUnlimitedCheck_->setChecked(s.seed_unlimited);
  }
  if (seedTargetRatioSpin_ != nullptr) {
    seedTargetRatioSpin_->setValue(s.seed_target_ratio);
  }
  if (seedMaxMinutesSpin_ != nullptr) {
    seedMaxMinutesSpin_->setValue(std::clamp(s.seed_max_minutes, 0, 10080));
    seedMaxMinutesSpin_->setEnabled(!s.seed_unlimited);
  }
  if (fileLogEnabledCheck_ != nullptr) {
    fileLogEnabledCheck_->setChecked(s.file_log_enabled);
  }
  if (logLevelBox_ != nullptr) {
    const QString lvl = s.log_level.trimmed().toLower();
    for (int i = 0; i < logLevelBox_->count(); ++i) {
      if (logLevelBox_->itemData(i).toString() == lvl) {
        logLevelBox_->setCurrentIndex(i);
        break;
      }
    }
  }
  if (logRotateEnabledCheck_ != nullptr) {
    logRotateEnabledCheck_->setChecked(s.log_rotate_max_file_size_bytes > 0);
  }
  if (logRotateMaxSizeMiB_ != nullptr) {
    const int mib = static_cast<int>(
        std::clamp<qint64>(s.log_rotate_max_file_size_bytes / (1024LL * 1024LL), 1, 4096));
    logRotateMaxSizeMiB_->setValue(mib);
    logRotateMaxSizeMiB_->setEnabled(s.log_rotate_max_file_size_bytes > 0);
  }
  if (logRotateBackups_ != nullptr) {
    logRotateBackups_->setValue(std::clamp(s.log_rotate_max_backup_files, 1, 50));
    logRotateBackups_->setEnabled(s.log_rotate_max_file_size_bytes > 0);
  }
  if (logDirEdit_ != nullptr) {
    logDirEdit_->setText(s.log_dir);
  }
  if (downloadLimitKiB_ != nullptr) {
    downloadLimitKiB_->setValue(std::max(0, s.download_rate_limit_kib));
  }
  if (uploadLimitKiB_ != nullptr) {
    uploadLimitKiB_->setValue(std::max(0, s.upload_rate_limit_kib));
  }
  if (connectionsLimit_ != nullptr) {
    connectionsLimit_->setValue(std::max(0, s.connections_limit));
  }
  if (perTorrentConnectionsLimit_ != nullptr) {
    perTorrentConnectionsLimit_->setValue(std::max(0, s.per_torrent_connections_limit));
  }
  if (uploadSlotsLimit_ != nullptr) {
    uploadSlotsLimit_->setValue(std::max(0, s.upload_slots_limit));
  }
  if (perTorrentUploadSlotsLimit_ != nullptr) {
    perTorrentUploadSlotsLimit_->setValue(std::max(0, s.per_torrent_upload_slots_limit));
  }
  if (listenPortSpin_ != nullptr) {
    listenPortSpin_->setValue(std::clamp(s.listen_port, 0, 65535));
  }
  if (listenPortForwardingCheck_ != nullptr) {
    listenPortForwardingCheck_->setChecked(s.listen_port_forwarding);
  }
  if (activeDownloads_ != nullptr) {
    activeDownloads_->setValue(std::max(0, s.active_downloads));
  }
  if (activeSeeds_ != nullptr) {
    activeSeeds_->setValue(std::max(0, s.active_seeds));
  }
  if (activeLimit_ != nullptr) {
    activeLimit_->setValue(std::max(0, s.active_limit));
  }
  if (preferSeedsCheck_ != nullptr) {
    preferSeedsCheck_->setChecked(s.auto_manage_prefer_seeds);
  }
  if (enableDhtCheck_ != nullptr) {
    enableDhtCheck_->setChecked(s.enable_dht);
  }
  if (enableUpnpCheck_ != nullptr) {
    enableUpnpCheck_->setChecked(s.enable_upnp);
  }
  if (enableNatpmpCheck_ != nullptr) {
    enableNatpmpCheck_->setChecked(s.enable_natpmp);
  }
  if (enableLsdCheck_ != nullptr) {
    enableLsdCheck_->setChecked(s.enable_lsd);
  }
  if (monitorPortSpin_ != nullptr) {
    monitorPortSpin_->setValue(std::clamp(s.monitor_port, 0, 65535));
  }
  if (builtinTrackerEnabledCheck_ != nullptr) {
    builtinTrackerEnabledCheck_->setChecked(s.builtin_tracker_enabled);
  }
  if (builtinTrackerPortSpin_ != nullptr) {
    builtinTrackerPortSpin_->setValue(std::clamp(s.builtin_tracker_port, 0, 65535));
    builtinTrackerPortSpin_->setEnabled(s.builtin_tracker_enabled);
  }
  if (builtinTrackerPortForwardingCheck_ != nullptr) {
    builtinTrackerPortForwardingCheck_->setChecked(s.builtin_tracker_port_forwarding);
    builtinTrackerPortForwardingCheck_->setEnabled(s.builtin_tracker_enabled);
  }
  if (encryptionModeBox_ != nullptr) {
    const QString v = s.encryption_mode.trimmed().toLower();
    for (int i = 0; i < encryptionModeBox_->count(); ++i) {
      if (encryptionModeBox_->itemData(i).toString() == v) {
        encryptionModeBox_->setCurrentIndex(i);
        break;
      }
    }
  }
  if (ipFilterEnabledCheck_ != nullptr) {
    ipFilterEnabledCheck_->setChecked(s.ip_filter_enabled);
  }
  if (ipFilterPathEdit_ != nullptr) {
    ipFilterPathEdit_->setText(s.ip_filter_path);
    ipFilterPathEdit_->setEnabled(s.ip_filter_enabled);
  }
  if (proxyEnabledCheck_ != nullptr) {
    proxyEnabledCheck_->setChecked(s.proxy_enabled);
  }
  if (proxyTypeBox_ != nullptr) {
    const QString type = s.proxy_type.trimmed().toLower();
    for (int i = 0; i < proxyTypeBox_->count(); ++i) {
      if (proxyTypeBox_->itemData(i).toString() == type) {
        proxyTypeBox_->setCurrentIndex(i);
        break;
      }
    }
    proxyTypeBox_->setEnabled(s.proxy_enabled);
  }
  if (proxyHostEdit_ != nullptr) {
    proxyHostEdit_->setText(s.proxy_host);
    proxyHostEdit_->setEnabled(s.proxy_enabled);
  }
  if (proxyPortSpin_ != nullptr) {
    proxyPortSpin_->setValue(std::clamp(s.proxy_port, 1, 65535));
    proxyPortSpin_->setEnabled(s.proxy_enabled);
  }
  if (proxyUsernameEdit_ != nullptr) {
    proxyUsernameEdit_->setText(s.proxy_username);
    proxyUsernameEdit_->setEnabled(s.proxy_enabled);
  }
  if (proxyPasswordEdit_ != nullptr) {
    proxyPasswordEdit_->setText(s.proxy_password);
    proxyPasswordEdit_->setEnabled(s.proxy_enabled);
  }
  if (closeBehaviorBox_ != nullptr) {
    const QString cb = s.close_behavior.trimmed().toLower();
    for (int i = 0; i < closeBehaviorBox_->count(); ++i) {
      if (closeBehaviorBox_->itemData(i).toString() == cb) {
        closeBehaviorBox_->setCurrentIndex(i);
        break;
      }
    }
  }
  if (startMinimizedCheck_ != nullptr) {
    startMinimizedCheck_->setChecked(s.start_minimized);
  }
  if (timedActionBox_ != nullptr) {
    const QString timedAction = s.timed_action.trimmed().toLower();
    for (int i = 0; i < timedActionBox_->count(); ++i) {
      if (timedActionBox_->itemData(i).toString() == timedAction) {
        timedActionBox_->setCurrentIndex(i);
        break;
      }
    }
  }
  if (timedActionDelaySpin_ != nullptr) {
    timedActionDelaySpin_->setValue(std::clamp(s.timed_action_delay_minutes, 0, 10080));
    const QString currentAction = timedActionBox_ != nullptr
                                      ? timedActionBox_->currentData().toString()
                                      : QStringLiteral("none");
    timedActionDelaySpin_->setEnabled(currentAction != QStringLiteral("none"));
  }
}

void SettingsDialog::setRssSettings(const pfd::core::rss::RssSettings& s) {
  if (rssGlobalAutoDownloadCheck_ != nullptr) {
    rssGlobalAutoDownloadCheck_->setChecked(s.global_auto_download);
  }
  if (rssRefreshIntervalSpin_ != nullptr) {
    rssRefreshIntervalSpin_->setValue(s.refresh_interval_minutes);
  }
  if (rssMaxAutoDownloadsSpin_ != nullptr) {
    rssMaxAutoDownloadsSpin_->setValue(s.max_auto_downloads_per_refresh);
  }
  if (rssHistoryMaxItemsSpin_ != nullptr) {
    rssHistoryMaxItemsSpin_->setValue(s.history_max_items);
  }
  if (rssHistoryMaxAgeSpin_ != nullptr) {
    rssHistoryMaxAgeSpin_->setValue(s.history_max_age_days);
  }
  if (rssPlayerCommandEdit_ != nullptr) {
    rssPlayerCommandEdit_->setText(s.external_player_command);
  }
}

void SettingsDialog::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 16);
  root->setSpacing(14);

  auto* tabs = new QTabWidget(this);
  tabs->setDocumentMode(true);
  tabs->setUsesScrollButtons(true);

  auto makeScrollableTab = [tabs](QWidget* inner, const QString& title) {
    auto* scroll = new QScrollArea(tabs);
    scroll->setWidget(inner);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tabs->addTab(scroll, title);
  };

  // --- 任务（下载 + 做种） ---
  auto* downloadTab = new QWidget(tabs);
  auto* downloadLayout = new QVBoxLayout(downloadTab);
  downloadLayout->setContentsMargins(12, 12, 12, 12);
  downloadLayout->setSpacing(12);

  auto* downloadGroup = new QGroupBox(QStringLiteral("下载"), downloadTab);
  auto* downloadForm = new QFormLayout(downloadGroup);
  downloadForm->setLabelAlignment(Qt::AlignLeft);
  downloadForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  downloadForm->setFormAlignment(Qt::AlignTop);
  downloadForm->setVerticalSpacing(10);

  auto* downloadDirRow = new QWidget(downloadGroup);
  auto* downloadDirRowLayout = new QHBoxLayout(downloadDirRow);
  downloadDirRowLayout->setContentsMargins(0, 0, 0, 0);
  downloadDirRowLayout->setSpacing(8);
  downloadDirEdit_ = new QLineEdit(downloadDirRow);
  downloadDirEdit_->setPlaceholderText(QStringLiteral("默认下载目录"));
  browseDownloadDirBtn_ = new QPushButton(QStringLiteral("浏览"), downloadDirRow);
  downloadDirRowLayout->addWidget(downloadDirEdit_, 1);
  downloadDirRowLayout->addWidget(browseDownloadDirBtn_);

  downloadForm->addRow(QStringLiteral("默认下载目录"), downloadDirRow);
  downloadLayout->addWidget(downloadGroup);

  auto* seedGroup = new QGroupBox(QStringLiteral("做种策略"), downloadTab);
  auto* seedForm = new QFormLayout(seedGroup);
  seedForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  seedForm->setFormAlignment(Qt::AlignTop);
  seedForm->setVerticalSpacing(10);

  seedUnlimitedCheck_ = new QCheckBox(QStringLiteral("无限做种（不自动停止）"), seedGroup);

  seedTargetRatioSpin_ = new QDoubleSpinBox(seedGroup);
  seedTargetRatioSpin_->setRange(0.10, 100.00);
  seedTargetRatioSpin_->setDecimals(2);
  seedTargetRatioSpin_->setSingleStep(0.10);
  seedTargetRatioSpin_->setValue(1.00);

  seedMaxMinutesSpin_ = new QSpinBox(seedGroup);
  seedMaxMinutesSpin_->setRange(0, 10080);
  seedMaxMinutesSpin_->setSuffix(QStringLiteral(" 分钟"));
  seedMaxMinutesSpin_->setSpecialValueText(QStringLiteral("不限制"));
  seedMaxMinutesSpin_->setSingleStep(30);
  seedMaxMinutesSpin_->setValue(0);

  seedForm->addRow(QStringLiteral("模式"), seedUnlimitedCheck_);
  seedForm->addRow(QStringLiteral("目标分享率"), seedTargetRatioSpin_);
  seedForm->addRow(QStringLiteral("做种时长上限"), seedMaxMinutesSpin_);
  downloadLayout->addWidget(seedGroup);
  auto* seedHint = new QLabel(
      QStringLiteral("提示：勾选「无限做种」时，分享率和时长上限均不生效。"), downloadTab);
  seedHint->setProperty("class", QStringLiteral("sectionHint"));
  downloadLayout->addWidget(seedHint);
  downloadLayout->addStretch(1);
  makeScrollableTab(downloadTab, QStringLiteral("任务"));

  // --- 日志 ---
  auto* logTab = new QWidget(tabs);
  auto* logLayout = new QVBoxLayout(logTab);
  logLayout->setContentsMargins(12, 12, 12, 12);
  logLayout->setSpacing(12);

  auto* logGroup = new QGroupBox(QStringLiteral("日志"), logTab);
  auto* logForm = new QFormLayout(logGroup);
  logForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  logForm->setFormAlignment(Qt::AlignTop);
  logForm->setVerticalSpacing(10);

  fileLogEnabledCheck_ = new QCheckBox(QStringLiteral("启用日志落盘"), logGroup);
  fileLogEnabledCheck_->setChecked(true);

  auto* logDirRow = new QWidget(logGroup);
  auto* logDirRowLayout = new QHBoxLayout(logDirRow);
  logDirRowLayout->setContentsMargins(0, 0, 0, 0);
  logDirRowLayout->setSpacing(8);
  logDirEdit_ = new QLineEdit(logDirRow);
  logDirEdit_->setPlaceholderText(QStringLiteral("日志保存目录"));
  browseLogDirBtn_ = new QPushButton(QStringLiteral("浏览"), logDirRow);
  logDirRowLayout->addWidget(logDirEdit_, 1);
  logDirRowLayout->addWidget(browseLogDirBtn_);

  logForm->addRow(QStringLiteral("开关"), fileLogEnabledCheck_);
  logLevelBox_ = new QComboBox(logGroup);
  logLevelBox_->addItem(QStringLiteral("调试（debug）"), QStringLiteral("debug"));
  logLevelBox_->addItem(QStringLiteral("信息（info）"), QStringLiteral("info"));
  logLevelBox_->addItem(QStringLiteral("警告（warning）"), QStringLiteral("warning"));
  logLevelBox_->addItem(QStringLiteral("错误（error）"), QStringLiteral("error"));
  logForm->addRow(QStringLiteral("日志级别"), logLevelBox_);

  logRotateEnabledCheck_ = new QCheckBox(QStringLiteral("启用日志轮转"), logGroup);
  logRotateEnabledCheck_->setChecked(false);

  logRotateMaxSizeMiB_ = new QSpinBox(logGroup);
  logRotateMaxSizeMiB_->setRange(1, 4096);
  logRotateMaxSizeMiB_->setSuffix(QStringLiteral(" MiB"));
  logRotateMaxSizeMiB_->setValue(32);

  logRotateBackups_ = new QSpinBox(logGroup);
  logRotateBackups_->setRange(1, 50);
  logRotateBackups_->setValue(3);

  logForm->addRow(QStringLiteral("轮转"), logRotateEnabledCheck_);
  logForm->addRow(QStringLiteral("单文件上限"), logRotateMaxSizeMiB_);
  logForm->addRow(QStringLiteral("保留份数"), logRotateBackups_);
  logForm->addRow(QStringLiteral("日志目录"), logDirRow);
  logLayout->addWidget(logGroup);
  auto* logHint = new QLabel(
      QStringLiteral("提示：修改日志目录后，需要重新打开程序才能确保所有组件都使用新路径。"),
      logTab);
  logHint->setProperty("class", QStringLiteral("sectionHint"));
  logLayout->addWidget(logHint);

  // --- 连接（网络 + 队列 + 协议） ---
  auto* netTab = new QWidget(tabs);
  auto* netLayout = new QVBoxLayout(netTab);
  netLayout->setContentsMargins(12, 12, 12, 12);
  netLayout->setSpacing(12);

  auto* netGroup = new QGroupBox(QStringLiteral("网络与限速"), netTab);
  auto* netForm = new QFormLayout(netGroup);
  netForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  netForm->setFormAlignment(Qt::AlignTop);
  netForm->setVerticalSpacing(10);

  downloadLimitKiB_ = new QSpinBox(netGroup);
  downloadLimitKiB_->setRange(0, 1024 * 1024);
  downloadLimitKiB_->setSuffix(QStringLiteral(" KiB/s"));
  downloadLimitKiB_->setValue(0);

  uploadLimitKiB_ = new QSpinBox(netGroup);
  uploadLimitKiB_->setRange(0, 1024 * 1024);
  uploadLimitKiB_->setSuffix(QStringLiteral(" KiB/s"));
  uploadLimitKiB_->setValue(0);

  connectionsLimit_ = new QSpinBox(netGroup);
  connectionsLimit_->setRange(0, 20000);
  connectionsLimit_->setValue(200);

  perTorrentConnectionsLimit_ = new QSpinBox(netGroup);
  perTorrentConnectionsLimit_->setRange(0, 20000);
  perTorrentConnectionsLimit_->setValue(0);
  uploadSlotsLimit_ = new QSpinBox(netGroup);
  uploadSlotsLimit_->setRange(0, 20000);
  uploadSlotsLimit_->setValue(0);
  perTorrentUploadSlotsLimit_ = new QSpinBox(netGroup);
  perTorrentUploadSlotsLimit_->setRange(0, 20000);
  perTorrentUploadSlotsLimit_->setValue(0);

  auto* listenPortRow = new QWidget(netGroup);
  auto* listenPortLayout = new QHBoxLayout(listenPortRow);
  listenPortLayout->setContentsMargins(0, 0, 0, 0);
  listenPortLayout->setSpacing(8);
  listenPortSpin_ = new QSpinBox(listenPortRow);
  listenPortSpin_->setRange(0, 65535);
  listenPortSpin_->setSpecialValueText(QStringLiteral("自动"));
  listenPortSpin_->setValue(0);
  randomListenPortBtn_ = new QPushButton(QStringLiteral("随机"), listenPortRow);
  randomListenPortBtn_->setToolTip(QStringLiteral("随机选择一个当前可用端口"));
  listenPortLayout->addWidget(listenPortSpin_, 1);
  listenPortLayout->addWidget(randomListenPortBtn_);
  listenPortForwardingCheck_ =
      new QCheckBox(QStringLiteral("使用我的路由器的 UPnP/NAT-PMP 端口转发"), netGroup);
  listenPortForwardingCheck_->setChecked(true);

  netForm->addRow(QStringLiteral("下载限速"), downloadLimitKiB_);
  netForm->addRow(QStringLiteral("上传限速"), uploadLimitKiB_);
  netForm->addRow(QStringLiteral("连接数上限"), connectionsLimit_);
  netForm->addRow(QStringLiteral("单任务连接数上限"), perTorrentConnectionsLimit_);
  netForm->addRow(QStringLiteral("全局上传窗口数上限"), uploadSlotsLimit_);
  netForm->addRow(QStringLiteral("每个 Torrent 上传窗口数上限"), perTorrentUploadSlotsLimit_);
  netForm->addRow(QStringLiteral("监听端口（传入连接）"), listenPortRow);
  netForm->addRow(QStringLiteral("端口转发"), listenPortForwardingCheck_);
  netLayout->addWidget(netGroup);
  auto* netHint = new QLabel(QStringLiteral("提示：0 表示使用默认值/不限制。"), netTab);
  netHint->setProperty("class", QStringLiteral("sectionHint"));
  netLayout->addWidget(netHint);

  auto* queueGroup = new QGroupBox(QStringLiteral("并发与队列"), netTab);
  auto* queueForm = new QFormLayout(queueGroup);
  queueForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  queueForm->setFormAlignment(Qt::AlignTop);
  queueForm->setVerticalSpacing(10);

  activeDownloads_ = new QSpinBox(queueGroup);
  activeDownloads_->setRange(0, 20000);
  activeDownloads_->setValue(0);
  activeSeeds_ = new QSpinBox(queueGroup);
  activeSeeds_->setRange(0, 20000);
  activeSeeds_->setValue(0);
  activeLimit_ = new QSpinBox(queueGroup);
  activeLimit_->setRange(0, 20000);
  activeLimit_->setValue(0);
  preferSeedsCheck_ = new QCheckBox(
      QStringLiteral("优先让做种任务保持活跃（auto_manage_prefer_seeds）"), queueGroup);

  queueForm->addRow(QStringLiteral("活跃下载数上限"), activeDownloads_);
  queueForm->addRow(QStringLiteral("活跃做种数上限"), activeSeeds_);
  queueForm->addRow(QStringLiteral("活跃任务总上限"), activeLimit_);
  queueForm->addRow(QStringLiteral("队列策略"), preferSeedsCheck_);
  netLayout->addWidget(queueGroup);
  auto* queueHint = new QLabel(QStringLiteral("提示：0 表示使用默认值/不限制。"), netTab);
  queueHint->setProperty("class", QStringLiteral("sectionHint"));
  netLayout->addWidget(queueHint);

  auto* discoverGroup = new QGroupBox(QStringLiteral("发现与端口映射"), netTab);
  auto* discoverForm = new QFormLayout(discoverGroup);
  discoverForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  discoverForm->setFormAlignment(Qt::AlignTop);
  discoverForm->setVerticalSpacing(10);
  enableDhtCheck_ = new QCheckBox(QStringLiteral("启用 DHT"), discoverGroup);
  enableUpnpCheck_ = new QCheckBox(QStringLiteral("启用 UPnP"), discoverGroup);
  enableNatpmpCheck_ = new QCheckBox(QStringLiteral("启用 NAT-PMP"), discoverGroup);
  enableLsdCheck_ = new QCheckBox(QStringLiteral("启用 LSD（本地服务发现）"), discoverGroup);
  enableDhtCheck_->setChecked(true);
  enableUpnpCheck_->setChecked(true);
  enableNatpmpCheck_->setChecked(true);
  enableLsdCheck_->setChecked(true);
  discoverForm->addRow(QStringLiteral("DHT"), enableDhtCheck_);
  discoverForm->addRow(QStringLiteral("UPnP"), enableUpnpCheck_);
  discoverForm->addRow(QStringLiteral("NAT-PMP"), enableNatpmpCheck_);
  discoverForm->addRow(QStringLiteral("LSD"), enableLsdCheck_);
  monitorPortSpin_ = new QSpinBox(discoverGroup);
  monitorPortSpin_->setRange(0, 65535);
  monitorPortSpin_->setSpecialValueText(QStringLiteral("禁用"));
  monitorPortSpin_->setValue(0);
  discoverForm->addRow(QStringLiteral("监控端口"), monitorPortSpin_);
  builtinTrackerEnabledCheck_ = new QCheckBox(QStringLiteral("启用内置 Tracker"), discoverGroup);
  builtinTrackerPortSpin_ = new QSpinBox(discoverGroup);
  builtinTrackerPortSpin_->setRange(0, 65535);
  builtinTrackerPortSpin_->setSpecialValueText(QStringLiteral("自动"));
  builtinTrackerPortSpin_->setValue(0);
  builtinTrackerPortForwardingCheck_ =
      new QCheckBox(QStringLiteral("对内置 Tracker 启用端口转发"), discoverGroup);
  discoverForm->addRow(QStringLiteral("内置 Tracker"), builtinTrackerEnabledCheck_);
  discoverForm->addRow(QStringLiteral("内置 Tracker 端口"), builtinTrackerPortSpin_);
  discoverForm->addRow(QStringLiteral("内置 Tracker 转发"), builtinTrackerPortForwardingCheck_);

  auto* encGroup = new QGroupBox(QStringLiteral("连接加密"), netTab);
  auto* encForm = new QFormLayout(encGroup);
  encForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  encForm->setFormAlignment(Qt::AlignTop);
  encForm->setVerticalSpacing(10);
  encryptionModeBox_ = new QComboBox(encGroup);
  encryptionModeBox_->addItem(QStringLiteral("启用（优先明文，兼容加密）"),
                              QStringLiteral("enabled"));
  encryptionModeBox_->addItem(QStringLiteral("强制（只允许加密连接）"), QStringLiteral("forced"));
  encryptionModeBox_->addItem(QStringLiteral("禁用（只允许明文连接）"), QStringLiteral("disabled"));
  encForm->addRow(QStringLiteral("策略"), encryptionModeBox_);
  auto* netAdvancedGroup = new QGroupBox(QStringLiteral("代理与 IP 过滤（重启后生效）"), netTab);
  auto* netAdvancedForm = new QFormLayout(netAdvancedGroup);
  netAdvancedForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  netAdvancedForm->setFormAlignment(Qt::AlignTop);
  netAdvancedForm->setVerticalSpacing(10);
  ipFilterEnabledCheck_ = new QCheckBox(QStringLiteral("启用 IP 过滤"), netAdvancedGroup);
  netAdvancedForm->addRow(QStringLiteral("IP 过滤"), ipFilterEnabledCheck_);
  ipFilterPathEdit_ = new QLineEdit(netAdvancedGroup);
  ipFilterPathEdit_->setPlaceholderText(QStringLiteral("过滤规则文件路径（CIDR/规则列表）"));
  netAdvancedForm->addRow(QStringLiteral("规则文件"), ipFilterPathEdit_);
  proxyEnabledCheck_ = new QCheckBox(QStringLiteral("启用代理"), netAdvancedGroup);
  netAdvancedForm->addRow(QStringLiteral("代理"), proxyEnabledCheck_);
  proxyTypeBox_ = new QComboBox(netAdvancedGroup);
  proxyTypeBox_->addItem(QStringLiteral("SOCKS5"), QStringLiteral("socks5"));
  proxyTypeBox_->addItem(QStringLiteral("HTTP"), QStringLiteral("http"));
  netAdvancedForm->addRow(QStringLiteral("代理类型"), proxyTypeBox_);
  proxyHostEdit_ = new QLineEdit(netAdvancedGroup);
  proxyHostEdit_->setPlaceholderText(QStringLiteral("例如：127.0.0.1"));
  netAdvancedForm->addRow(QStringLiteral("代理主机"), proxyHostEdit_);
  proxyPortSpin_ = new QSpinBox(netAdvancedGroup);
  proxyPortSpin_->setRange(1, 65535);
  proxyPortSpin_->setValue(1080);
  netAdvancedForm->addRow(QStringLiteral("代理端口"), proxyPortSpin_);
  proxyUsernameEdit_ = new QLineEdit(netAdvancedGroup);
  proxyUsernameEdit_->setPlaceholderText(QStringLiteral("可选"));
  netAdvancedForm->addRow(QStringLiteral("用户名"), proxyUsernameEdit_);
  proxyPasswordEdit_ = new QLineEdit(netAdvancedGroup);
  proxyPasswordEdit_->setEchoMode(QLineEdit::Password);
  proxyPasswordEdit_->setPlaceholderText(QStringLiteral("可选"));
  netAdvancedForm->addRow(QStringLiteral("密码"), proxyPasswordEdit_);
  netLayout->addWidget(discoverGroup);
  netLayout->addWidget(encGroup);
  netLayout->addWidget(netAdvancedGroup);
  netLayout->addStretch(1);
  makeScrollableTab(netTab, QStringLiteral("连接"));

  // --- RSS ---
  auto* rssTab = new QWidget(tabs);
  auto* rssLayout = new QVBoxLayout(rssTab);
  rssLayout->setContentsMargins(12, 12, 12, 12);
  rssLayout->setSpacing(12);

  auto* rssDlGroup = new QGroupBox(QStringLiteral("RSS 自动下载"), rssTab);
  auto* rssDlForm = new QFormLayout(rssDlGroup);
  rssDlForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  rssDlForm->setFormAlignment(Qt::AlignTop);
  rssDlForm->setVerticalSpacing(10);
  rssGlobalAutoDownloadCheck_ =
      new QCheckBox(QStringLiteral("启用全局自动下载（总开关）"), rssDlGroup);
  rssDlForm->addRow(rssGlobalAutoDownloadCheck_);
  rssMaxAutoDownloadsSpin_ = new QSpinBox(rssDlGroup);
  rssMaxAutoDownloadsSpin_->setRange(1, 100);
  rssMaxAutoDownloadsSpin_->setSuffix(QStringLiteral(" 条"));
  rssDlForm->addRow(QStringLiteral("每次刷新最大自动下载数"), rssMaxAutoDownloadsSpin_);
  rssLayout->addWidget(rssDlGroup);

  auto* rssRefreshGroup = new QGroupBox(QStringLiteral("RSS 刷新"), rssTab);
  auto* rssRefreshForm = new QFormLayout(rssRefreshGroup);
  rssRefreshForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  rssRefreshForm->setFormAlignment(Qt::AlignTop);
  rssRefreshForm->setVerticalSpacing(10);
  rssRefreshIntervalSpin_ = new QSpinBox(rssRefreshGroup);
  rssRefreshIntervalSpin_->setRange(5, 1440);
  rssRefreshIntervalSpin_->setSuffix(QStringLiteral(" 分钟"));
  rssRefreshForm->addRow(QStringLiteral("自动刷新间隔"), rssRefreshIntervalSpin_);
  rssLayout->addWidget(rssRefreshGroup);

  auto* rssHistGroup = new QGroupBox(QStringLiteral("RSS 历史与清理"), rssTab);
  auto* rssHistForm = new QFormLayout(rssHistGroup);
  rssHistForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  rssHistForm->setFormAlignment(Qt::AlignTop);
  rssHistForm->setVerticalSpacing(10);
  rssHistoryMaxItemsSpin_ = new QSpinBox(rssHistGroup);
  rssHistoryMaxItemsSpin_->setRange(100, 100000);
  rssHistoryMaxItemsSpin_->setSuffix(QStringLiteral(" 条"));
  rssHistForm->addRow(QStringLiteral("最大保留条目数"), rssHistoryMaxItemsSpin_);
  rssHistoryMaxAgeSpin_ = new QSpinBox(rssHistGroup);
  rssHistoryMaxAgeSpin_->setRange(0, 3650);
  rssHistoryMaxAgeSpin_->setSpecialValueText(QStringLiteral("不限制"));
  rssHistoryMaxAgeSpin_->setSuffix(QStringLiteral(" 天"));
  rssHistForm->addRow(QStringLiteral("最长保留天数"), rssHistoryMaxAgeSpin_);
  rssLayout->addWidget(rssHistGroup);

  auto* rssPlayerGroup = new QGroupBox(QStringLiteral("RSS 外部播放器"), rssTab);
  auto* rssPlayerForm = new QFormLayout(rssPlayerGroup);
  rssPlayerForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  rssPlayerForm->setFormAlignment(Qt::AlignTop);
  rssPlayerForm->setVerticalSpacing(10);
  rssPlayerCommandEdit_ = new QLineEdit(rssPlayerGroup);
  rssPlayerCommandEdit_->setPlaceholderText(
      QStringLiteral("留空使用系统默认播放器，例如：mpv, vlc"));
  rssPlayerForm->addRow(QStringLiteral("播放器命令"), rssPlayerCommandEdit_);
  rssLayout->addWidget(rssPlayerGroup);

  auto* rssHint = new QLabel(
      QStringLiteral("提示：订阅源、规则、剧集页中的行为仍在各自页面配置，本页为 RSS 全局行为。"),
      rssTab);
  rssHint->setProperty("class", QStringLiteral("sectionHint"));
  rssLayout->addWidget(rssHint);
  rssLayout->addStretch(1);
  makeScrollableTab(rssTab, QStringLiteral("内容"));

  // --- 系统（日志 + 高级） ---
  auto* advancedTab = logTab;
  auto* advancedLayout = logLayout;

  auto* trackerGroup = new QGroupBox(QStringLiteral("默认 Tracker"), advancedTab);
  auto* trackerForm = new QFormLayout(trackerGroup);
  trackerForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  trackerForm->setFormAlignment(Qt::AlignTop);
  trackerForm->setVerticalSpacing(10);
  autoApplyTrackersCheck_ =
      new QCheckBox(QStringLiteral("为新任务自动附加默认 Tracker 列表"), trackerGroup);
  trackerForm->addRow(autoApplyTrackersCheck_);
  defaultTrackersEdit_ = new QPlainTextEdit(trackerGroup);
  defaultTrackersEdit_->setPlaceholderText(
      QStringLiteral("每行一个 Tracker URL\nhttp://tracker.example.com/announce"));
  defaultTrackersEdit_->setMaximumHeight(120);
  trackerForm->addRow(QStringLiteral("Tracker 列表"), defaultTrackersEdit_);
  advancedLayout->addWidget(trackerGroup);

  auto* behaviorGroup = new QGroupBox(QStringLiteral("窗口行为"), advancedTab);
  auto* behaviorForm = new QFormLayout(behaviorGroup);
  behaviorForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  behaviorForm->setFormAlignment(Qt::AlignTop);
  behaviorForm->setVerticalSpacing(10);
  closeBehaviorBox_ = new QComboBox(behaviorGroup);
  closeBehaviorBox_->addItem(QStringLiteral("最小化到托盘"), QStringLiteral("minimize"));
  closeBehaviorBox_->addItem(QStringLiteral("直接退出"), QStringLiteral("quit"));
  behaviorForm->addRow(QStringLiteral("关闭窗口时"), closeBehaviorBox_);
  startMinimizedCheck_ =
      new QCheckBox(QStringLiteral("启动时最小化到托盘（下次启动生效）"), behaviorGroup);
  behaviorForm->addRow(QStringLiteral("启动行为"), startMinimizedCheck_);

  timedActionBox_ = new QComboBox(behaviorGroup);
  timedActionBox_->addItem(QStringLiteral("不执行"), QStringLiteral("none"));
  timedActionBox_->addItem(QStringLiteral("退出应用"), QStringLiteral("quit_app"));
  timedActionBox_->addItem(QStringLiteral("关机"), QStringLiteral("shutdown"));
  behaviorForm->addRow(QStringLiteral("定时动作"), timedActionBox_);

  timedActionDelaySpin_ = new QSpinBox(behaviorGroup);
  timedActionDelaySpin_->setRange(0, 10080);
  timedActionDelaySpin_->setSpecialValueText(QStringLiteral("禁用"));
  timedActionDelaySpin_->setSuffix(QStringLiteral(" 分钟后"));
  timedActionDelaySpin_->setValue(0);
  behaviorForm->addRow(QStringLiteral("执行时间"), timedActionDelaySpin_);
  advancedLayout->addWidget(behaviorGroup);

  auto* uiGroup = new QGroupBox(QStringLiteral("界面与性能"), advancedTab);
  auto* uiForm = new QFormLayout(uiGroup);
  uiForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  uiForm->setFormAlignment(Qt::AlignTop);
  uiForm->setVerticalSpacing(10);
  magnetMaxInflightSpin_ = new QSpinBox(uiGroup);
  magnetMaxInflightSpin_->setRange(5, 10);
  magnetMaxInflightSpin_->setValue(8);
  uiForm->addRow(QStringLiteral("磁力并发元数据数"), magnetMaxInflightSpin_);
  bottomStatusEnabledCheck_ = new QCheckBox(QStringLiteral("启用底部状态栏"), uiGroup);
  bottomStatusEnabledCheck_->setChecked(true);
  uiForm->addRow(QStringLiteral("状态栏"), bottomStatusEnabledCheck_);
  bottomStatusRefreshMsSpin_ = new QSpinBox(uiGroup);
  bottomStatusRefreshMsSpin_->setRange(500, 5000);
  bottomStatusRefreshMsSpin_->setSuffix(QStringLiteral(" ms"));
  bottomStatusRefreshMsSpin_->setSingleStep(100);
  bottomStatusRefreshMsSpin_->setValue(1000);
  uiForm->addRow(QStringLiteral("状态栏刷新间隔"), bottomStatusRefreshMsSpin_);
  advancedLayout->addWidget(uiGroup);

  auto* persistGroup = new QGroupBox(QStringLiteral("持久化与恢复"), advancedTab);
  auto* persistForm = new QFormLayout(persistGroup);
  persistForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  persistForm->setFormAlignment(Qt::AlignTop);
  persistForm->setVerticalSpacing(10);
  taskAutoSaveMsSpin_ = new QSpinBox(persistGroup);
  taskAutoSaveMsSpin_->setRange(2000, 60000);
  taskAutoSaveMsSpin_->setSuffix(QStringLiteral(" ms"));
  taskAutoSaveMsSpin_->setSingleStep(1000);
  taskAutoSaveMsSpin_->setValue(5000);
  persistForm->addRow(QStringLiteral("任务自动保存间隔"), taskAutoSaveMsSpin_);
  restoreStartPausedCheck_ =
      new QCheckBox(QStringLiteral("启动时恢复的任务默认暂停（下次启动生效）"), persistGroup);
  restoreStartPausedCheck_->setChecked(true);
  persistForm->addRow(QStringLiteral("恢复策略"), restoreStartPausedCheck_);
  advancedLayout->addWidget(persistGroup);

  auto* advancedHint =
      new QLabel(QStringLiteral("提示：底部状态栏开关与刷新间隔需要重启程序生效。"), advancedTab);
  advancedHint->setProperty("class", QStringLiteral("sectionHint"));
  advancedLayout->addWidget(advancedHint);
  advancedLayout->addStretch(1);
  makeScrollableTab(logTab, QStringLiteral("系统"));

  root->addWidget(tabs, 1);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  if (auto* okBtn = buttons->button(QDialogButtonBox::Ok); okBtn != nullptr) {
    okBtn->setText(QStringLiteral("确定"));
  }
  if (auto* cancelBtn = buttons->button(QDialogButtonBox::Cancel); cancelBtn != nullptr) {
    cancelBtn->setText(QStringLiteral("取消"));
  }
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);
}

void SettingsDialog::wireSignals() {
  if (browseDownloadDirBtn_ != nullptr) {
    connect(browseDownloadDirBtn_, &QPushButton::clicked, this, [this]() { browseDownloadDir(); });
  }
  if (browseLogDirBtn_ != nullptr) {
    connect(browseLogDirBtn_, &QPushButton::clicked, this, [this]() { browseLogDir(); });
  }
  if (seedUnlimitedCheck_ != nullptr && seedTargetRatioSpin_ != nullptr) {
    connect(seedUnlimitedCheck_, &QCheckBox::toggled, this, [this](bool checked) {
      seedTargetRatioSpin_->setEnabled(!checked);
      if (seedMaxMinutesSpin_ != nullptr) {
        seedMaxMinutesSpin_->setEnabled(!checked);
      }
    });
    seedTargetRatioSpin_->setEnabled(!seedUnlimitedCheck_->isChecked());
    if (seedMaxMinutesSpin_ != nullptr) {
      seedMaxMinutesSpin_->setEnabled(!seedUnlimitedCheck_->isChecked());
    }
  }
  if (autoApplyTrackersCheck_ != nullptr && defaultTrackersEdit_ != nullptr) {
    connect(autoApplyTrackersCheck_, &QCheckBox::toggled, this,
            [this](bool checked) { defaultTrackersEdit_->setEnabled(checked); });
    defaultTrackersEdit_->setEnabled(autoApplyTrackersCheck_->isChecked());
  }
  if (bottomStatusEnabledCheck_ != nullptr && bottomStatusRefreshMsSpin_ != nullptr) {
    connect(bottomStatusEnabledCheck_, &QCheckBox::toggled, this,
            [this](bool checked) { bottomStatusRefreshMsSpin_->setEnabled(checked); });
    bottomStatusRefreshMsSpin_->setEnabled(bottomStatusEnabledCheck_->isChecked());
  }
  if (logRotateEnabledCheck_ != nullptr && logRotateMaxSizeMiB_ != nullptr &&
      logRotateBackups_ != nullptr) {
    connect(logRotateEnabledCheck_, &QCheckBox::toggled, this, [this](bool checked) {
      logRotateMaxSizeMiB_->setEnabled(checked);
      logRotateBackups_->setEnabled(checked);
    });
    const bool enabled = logRotateEnabledCheck_->isChecked();
    logRotateMaxSizeMiB_->setEnabled(enabled);
    logRotateBackups_->setEnabled(enabled);
  }
  if (timedActionBox_ != nullptr && timedActionDelaySpin_ != nullptr) {
    connect(timedActionBox_, &QComboBox::currentIndexChanged, this, [this](int) {
      const QString action = timedActionBox_->currentData().toString();
      timedActionDelaySpin_->setEnabled(action != QStringLiteral("none"));
    });
    timedActionDelaySpin_->setEnabled(timedActionBox_->currentData().toString() !=
                                      QStringLiteral("none"));
  }
  if (ipFilterEnabledCheck_ != nullptr && ipFilterPathEdit_ != nullptr) {
    connect(ipFilterEnabledCheck_, &QCheckBox::toggled, this,
            [this](bool checked) { ipFilterPathEdit_->setEnabled(checked); });
    ipFilterPathEdit_->setEnabled(ipFilterEnabledCheck_->isChecked());
  }
  if (proxyEnabledCheck_ != nullptr) {
    connect(proxyEnabledCheck_, &QCheckBox::toggled, this, [this](bool checked) {
      if (proxyTypeBox_ != nullptr)
        proxyTypeBox_->setEnabled(checked);
      if (proxyHostEdit_ != nullptr)
        proxyHostEdit_->setEnabled(checked);
      if (proxyPortSpin_ != nullptr)
        proxyPortSpin_->setEnabled(checked);
      if (proxyUsernameEdit_ != nullptr)
        proxyUsernameEdit_->setEnabled(checked);
      if (proxyPasswordEdit_ != nullptr)
        proxyPasswordEdit_->setEnabled(checked);
    });
    const bool enabled = proxyEnabledCheck_->isChecked();
    if (proxyTypeBox_ != nullptr)
      proxyTypeBox_->setEnabled(enabled);
    if (proxyHostEdit_ != nullptr)
      proxyHostEdit_->setEnabled(enabled);
    if (proxyPortSpin_ != nullptr)
      proxyPortSpin_->setEnabled(enabled);
    if (proxyUsernameEdit_ != nullptr)
      proxyUsernameEdit_->setEnabled(enabled);
    if (proxyPasswordEdit_ != nullptr)
      proxyPasswordEdit_->setEnabled(enabled);
  }
  if (builtinTrackerEnabledCheck_ != nullptr) {
    connect(builtinTrackerEnabledCheck_, &QCheckBox::toggled, this, [this](bool checked) {
      if (builtinTrackerPortSpin_ != nullptr)
        builtinTrackerPortSpin_->setEnabled(checked);
      if (builtinTrackerPortForwardingCheck_ != nullptr)
        builtinTrackerPortForwardingCheck_->setEnabled(checked);
    });
    const bool enabled = builtinTrackerEnabledCheck_->isChecked();
    if (builtinTrackerPortSpin_ != nullptr)
      builtinTrackerPortSpin_->setEnabled(enabled);
    if (builtinTrackerPortForwardingCheck_ != nullptr)
      builtinTrackerPortForwardingCheck_->setEnabled(enabled);
  }
  if (randomListenPortBtn_ != nullptr && listenPortSpin_ != nullptr) {
    connect(randomListenPortBtn_, &QPushButton::clicked, this, [this]() {
      QTcpServer probe;
      if (probe.listen(QHostAddress::AnyIPv4, 0)) {
        listenPortSpin_->setValue(static_cast<int>(probe.serverPort()));
        probe.close();
      }
    });
  }
}

void SettingsDialog::browseDownloadDir() {
  const QString base = downloadDirEdit_ != nullptr ? downloadDirEdit_->text().trimmed() : QString();
  const QString dir =
      QFileDialog::getExistingDirectory(this, QStringLiteral("选择默认下载目录"), base);
  if (!dir.isEmpty() && downloadDirEdit_ != nullptr) {
    downloadDirEdit_->setText(dir);
  }
}

void SettingsDialog::browseLogDir() {
  const QString base = logDirEdit_ != nullptr ? logDirEdit_->text().trimmed() : QString();
  const QString dir =
      QFileDialog::getExistingDirectory(this, QStringLiteral("选择日志保存目录"), base);
  if (!dir.isEmpty() && logDirEdit_ != nullptr) {
    logDirEdit_->setText(dir);
  }
}

}  // namespace pfd::ui
