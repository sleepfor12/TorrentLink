#include <QApplication>
#include <QDir>
#include <QString>

#include <libtorrent/version.hpp>

#include "app_controller.h"
#include "base/types.h"
#include "core/config_service.h"
#include "core/logger.h"
#include "core/task_pipeline_service.h"
#include "lt/session_worker.h"
#include "ui/main_window.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  // 让 QSettings/QStandardPaths 拥有稳定的应用标识
  QCoreApplication::setOrganizationName(QStringLiteral("TorrentLink"));
  QCoreApplication::setApplicationName(QStringLiteral("TorrentLink"));

  const auto st = pfd::core::ConfigService::loadAppSettings();
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

  pfd::ui::MainWindow w;
  w.show();
  pfd::core::TaskPipelineService pipelineService;
  pfd::lt::SessionWorker worker;
  LOG_INFO(QStringLiteral("[main] libtorrent version: %1").arg(LIBTORRENT_VERSION));
  pfd::app::AppController controller(&app, &w, &pipelineService, &worker);
  controller.initialize();

  if (argc >= 2) {
    controller.submitArgvMagnet(QString::fromLocal8Bit(argv[1]));
  }

  return app.exec();
}
