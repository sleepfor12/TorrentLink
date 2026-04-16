#include "ui/log_center_dialog.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>

#include "base/types.h"
#include "core/config_service.h"
#include "core/logger.h"
#include "ui/input_ime_utils.h"
#include "ui/state/state_store.h"

namespace pfd::ui {

namespace {

QString levelText(pfd::base::LogLevel level) {
  return pfd::base::logLevelToString(level).toString();
}

bool passLevel(const pfd::core::LogEntry& e, const QString& level) {
  if (level == QStringLiteral("all")) {
    return true;
  }
  return levelText(e.level) == level;
}

}  // namespace

LogCenterDialog::LogCenterDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("日志中心"));
  resize(920, 580);
  buildLayout();
  bindSignals();
  const auto st = StateStore::loadLogCenterState();
  if (!st.geometry.isEmpty()) {
    restoreGeometry(st.geometry);
  }
  if (keywordFilter_ != nullptr) {
    const QSignalBlocker blocker(keywordFilter_);
    keywordFilter_->setText(st.keyword);
  }
  if (levelFilter_ != nullptr) {
    const int idx = levelFilter_->findData(st.level.isEmpty() ? QStringLiteral("all") : st.level);
    if (idx >= 0) {
      levelFilter_->setCurrentIndex(idx);
    }
  }
  reloadLogs();
}

void LogCenterDialog::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(10);

  auto* bar = new QHBoxLayout();
  bar->setSpacing(8);
  bar->addWidget(new QLabel(QStringLiteral("级别"), this));
  levelFilter_ = new QComboBox(this);
  levelFilter_->addItem(QStringLiteral("全部"), QStringLiteral("all"));
  levelFilter_->addItem(QStringLiteral("debug"), QStringLiteral("debug"));
  levelFilter_->addItem(QStringLiteral("info"), QStringLiteral("info"));
  levelFilter_->addItem(QStringLiteral("warning"), QStringLiteral("warning"));
  levelFilter_->addItem(QStringLiteral("error"), QStringLiteral("error"));
  levelFilter_->addItem(QStringLiteral("fatal"), QStringLiteral("fatal"));
  bar->addWidget(levelFilter_);

  bar->addWidget(new QLabel(QStringLiteral("关键字"), this));
  keywordFilter_ = new QLineEdit(this);
  keywordFilter_->setPlaceholderText(QStringLiteral("输入关键字过滤 message/detail"));
  bar->addWidget(keywordFilter_, 1);
  bar->addWidget(new QLabel(QStringLiteral("最大行数"), this));
  maxRowsFilter_ = new QComboBox(this);
  maxRowsFilter_->addItem(QStringLiteral("500"), 500);
  maxRowsFilter_->addItem(QStringLiteral("1000"), 1000);
  maxRowsFilter_->addItem(QStringLiteral("2000"), 2000);
  maxRowsFilter_->addItem(QStringLiteral("5000"), 5000);
  maxRowsFilter_->setCurrentIndex(3);
  bar->addWidget(maxRowsFilter_);
  newestFirstCheck_ = new QCheckBox(QStringLiteral("最新优先"), this);
  newestFirstCheck_->setChecked(true);
  bar->addWidget(newestFirstCheck_);

  refreshBtn_ = new QPushButton(QStringLiteral("刷新"), this);
  exportBtn_ = new QPushButton(QStringLiteral("导出"), this);
  clearBtn_ = new QPushButton(QStringLiteral("清理"), this);
  openDirBtn_ = new QPushButton(QStringLiteral("打开日志目录"), this);
  bar->addWidget(refreshBtn_);
  bar->addWidget(exportBtn_);
  bar->addWidget(clearBtn_);
  bar->addWidget(openDirBtn_);
  root->addLayout(bar);

  contentView_ = new QPlainTextEdit(this);
  contentView_->setReadOnly(true);
  root->addWidget(contentView_, 1);
}

void LogCenterDialog::bindSignals() {
  connect(levelFilter_, &QComboBox::currentIndexChanged, this, [this](int) { reloadLogs(); });
  connect(maxRowsFilter_, &QComboBox::currentIndexChanged, this, [this](int) { reloadLogs(); });
  connect(newestFirstCheck_, &QCheckBox::toggled, this, [this](bool) { reloadLogs(); });
  connectImeAwareLineEditApply(this, keywordFilter_, 180, [this]() { reloadLogs(); });
  connect(refreshBtn_, &QPushButton::clicked, this, [this]() { reloadLogs(); });
  connect(exportBtn_, &QPushButton::clicked, this, [this]() { exportLogs(); });
  connect(clearBtn_, &QPushButton::clicked, this, [this]() { clearLogs(); });
  connect(openDirBtn_, &QPushButton::clicked, this, [this]() { openLogDir(); });
  connect(this, &QDialog::finished, this, [this](int) {
    LogCenterUiState st;
    st.geometry = saveGeometry();
    st.level =
        levelFilter_ != nullptr ? levelFilter_->currentData().toString() : QStringLiteral("all");
    st.keyword = keywordFilter_ != nullptr ? keywordFilter_->text().trimmed() : QString();
    StateStore::saveLogCenterState(st);
  });
}

void LogCenterDialog::reloadLogs() {
  if (contentView_ != nullptr) {
    contentView_->setPlainText(buildFilteredLines().join('\n'));
  }
}

QStringList LogCenterDialog::buildFilteredLines() const {
  const QString level =
      levelFilter_ != nullptr ? levelFilter_->currentData().toString() : QStringLiteral("all");
  const QString keyword = keywordFilter_ != nullptr ? keywordFilter_->text().trimmed() : QString();
  const int maxRows = maxRowsFilter_ != nullptr ? maxRowsFilter_->currentData().toInt() : 5000;
  const bool newestFirst = newestFirstCheck_ != nullptr && newestFirstCheck_->isChecked();
  const auto logs = pfd::core::logger::Logger::instance().globalSnapshot(
      static_cast<std::size_t>(std::max(100, maxRows)));
  QStringList lines;
  lines.reserve(static_cast<int>(logs.size()));

  if (newestFirst) {
    for (auto it = logs.crbegin(); it != logs.crend(); ++it) {
      const auto& e = *it;
      if (!passLevel(e, level)) {
        continue;
      }
      const QString msg = e.message;
      const QString detail = e.detail;
      if (!keyword.isEmpty() && !msg.contains(keyword, Qt::CaseInsensitive) &&
          !detail.contains(keyword, Qt::CaseInsensitive)) {
        continue;
      }
      const QString ts = QDateTime::fromMSecsSinceEpoch(e.timestampMs)
                             .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
      lines.push_back(QStringLiteral("[%1] [%2] %3 %4")
                          .arg(ts, levelText(e.level), msg, detail.isEmpty() ? QString() : detail));
      if (maxRows > 0 && lines.size() >= maxRows) {
        return lines;
      }
    }
    return lines;
  }
  for (const auto& e : logs) {
    if (!passLevel(e, level)) {
      continue;
    }
    const QString msg = e.message;
    const QString detail = e.detail;
    if (!keyword.isEmpty() && !msg.contains(keyword, Qt::CaseInsensitive) &&
        !detail.contains(keyword, Qt::CaseInsensitive)) {
      continue;
    }
    const QString ts = QDateTime::fromMSecsSinceEpoch(e.timestampMs)
                           .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    lines.push_back(QStringLiteral("[%1] [%2] %3 %4")
                        .arg(ts, levelText(e.level), msg, detail.isEmpty() ? QString() : detail));
    if (maxRows > 0 && lines.size() >= maxRows) {
      break;
    }
  }
  return lines;
}

void LogCenterDialog::exportLogs() {
  const QString path = QFileDialog::getSaveFileName(
      this, QStringLiteral("导出日志"), QStringLiteral("logs_export.txt"),
      QStringLiteral("Text 文件 (*.txt);;所有文件 (*)"));
  if (path.isEmpty() || contentView_ == nullptr) {
    return;
  }
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("无法写入目标文件。"));
    return;
  }
  QTextStream out(&f);
  out.setEncoding(QStringConverter::Utf8);
  out << buildFilteredLines().join('\n');
  f.close();
  QMessageBox::information(this, QStringLiteral("导出完成"), QStringLiteral("日志已导出。"));
}

void LogCenterDialog::clearLogs() {
  if (QMessageBox::question(this, QStringLiteral("确认清理"),
                            QStringLiteral("确认清空内存中的全局日志吗？")) != QMessageBox::Yes) {
    return;
  }
  pfd::core::logger::Logger::instance().clearGlobalLogs();
  reloadLogs();
}

void LogCenterDialog::openLogDir() {
  const auto st = pfd::core::ConfigService::loadAppSettings();
  const QString dir = st.log_dir.trimmed();
  if (dir.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("打开失败"), QStringLiteral("日志目录为空。"));
    return;
  }
  QDir().mkpath(dir);
  QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

}  // namespace pfd::ui
