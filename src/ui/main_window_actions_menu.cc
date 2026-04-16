#include <QtCore/QDir>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include "core/config_service.h"
#include "core/logger.h"
#include "core/rss/rss_service.h"
#include "ui/add_torrent_dialog.h"
#include "ui/log_center_dialog.h"
#include "ui/main_window.h"
#include "ui/settings_dialog.h"

namespace pfd::ui {

void MainWindow::loadSettingsToUi() {
  // Settings are applied via SettingsDialog; nothing to sync to main window widgets.
}

void MainWindow::openPreferences() {
  SettingsDialog dlg(this);
  const auto combined = pfd::core::ConfigService::loadCombinedSettings();
  dlg.setSettings(combined.app);
  dlg.setControllerSettings(combined.controller);
  if (rssService_ != nullptr) {
    dlg.setRssSettings(rssService_->settings());
  }
  if (dlg.exec() != QDialog::Accepted) {
    return;
  }
  pfd::core::CombinedSettings toSave;
  toSave.app = dlg.currentSettings();
  toSave.controller = dlg.currentControllerSettings();
  const auto saveResult = pfd::core::ConfigService::saveCombinedSettings(toSave);
  if (!saveResult.ok) {
    LOG_ERROR(QStringLiteral("[ui] 保存首选项失败：%1").arg(saveResult.message));
    QMessageBox::warning(this, QStringLiteral("保存失败"),
                         QStringLiteral("配置保存失败：%1").arg(saveResult.message));
    return;
  }
  if (rssService_ != nullptr) {
    rssService_->applySettings(dlg.currentRssSettings());
    rssService_->saveState();
    emit rssSettingsChanged();
  }
  loadSettingsToUi();

  const auto& st = toSave.app;
  QDir().mkpath(st.log_dir);
  const QString logFile = QDir(st.log_dir).filePath(QStringLiteral("app.log"));
  pfd::core::logger::Logger::instance().setLogFilePath(st.file_log_enabled ? logFile : QString());
  pfd::core::logger::Logger::instance().setFileOutputEnabled(st.file_log_enabled);
  pfd::base::LogLevel level = pfd::base::LogLevel::kInfo;
  if (pfd::base::tryParseLogLevel(st.log_level, &level)) {
    pfd::core::logger::Logger::instance().setLogLevel(level);
  }
  pfd::core::logger::Logger::instance().setRotationPolicy(st.log_rotate_max_file_size_bytes,
                                                          st.log_rotate_max_backup_files);
  appendLog(QStringLiteral("设置已保存。"));
  emit settingsChanged();
}

void MainWindow::openLogCenter() {
  LogCenterDialog dlg(this);
  dlg.exec();
}

void MainWindow::openTorrentFileFromMenu() {
  const QString path =
      QFileDialog::getOpenFileName(this, QStringLiteral("选择 Torrent 文件"), QString(),
                                   QStringLiteral("Torrent 文件 (*.torrent);;所有文件 (*)"));
  if (path.isEmpty()) {
    return;
  }
  const auto st = pfd::core::ConfigService::loadAppSettings();
  const auto res = AddTorrentDialog::runForTorrentFile(this, path, st.default_download_dir);
  if (!res.has_value()) {
    appendLog(QStringLiteral("已取消添加 Torrent：%1").arg(path));
    return;
  }
  if (onAddTorrentFileRequest_) {
    onAddTorrentFileRequest_(AddTorrentFileRequest{path, *res});
    appendLog(QStringLiteral("已提交 Torrent：%1 保存到：%2").arg(res->name, res->savePath));
    return;
  }
  if (onAddTorrentFile_) {  // fallback
    onAddTorrentFile_(path, res->savePath);
    appendLog(
        QStringLiteral("已提交 Torrent（简化模式）：%1 保存到：%2").arg(res->name, res->savePath));
  }
}

}  // namespace pfd::ui
