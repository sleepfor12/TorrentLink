#include "core/logger.h"

#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileDevice>
#include <QtCore/QFileInfo>

#include <cstdlib>
#include <cstring>
#include <type_traits>

#include "base/paths.h"
#include "base/time_format.h"

namespace pfd::core::logger {

namespace {

const char* basenameSourceFile(const char* file) {
  if (file == nullptr || *file == '\0') {
    return "";
  }
  const char* slash = std::strrchr(file, '/');
  const char* bslash = std::strrchr(file, '\\');
  const char* last = slash;
  if (bslash != nullptr && (last == nullptr || bslash > last)) {
    last = bslash;
  }
  return last != nullptr ? last + 1 : file;
}

QString makeDetail(const char* file, int line, const char* func) {
  return QStringLiteral("%1:%2 %3")
      .arg(QString::fromUtf8(basenameSourceFile(file)))
      .arg(line)
      .arg(QString::fromUtf8(func ? func : ""));
}

QString datedLogPath(const QString& raw_path) {
  if (raw_path.isEmpty()) {
    return raw_path;
  }
  const QFileInfo fi(raw_path);
  const QString dir = fi.absolutePath();
  const QString base = fi.completeBaseName();
  const QString suffix = fi.suffix();
  const QString date = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));

  QString file_name = base + QStringLiteral("-") + date;
  if (!suffix.isEmpty()) {
    file_name += QStringLiteral(".") + suffix;
  }
  return QDir(dir).filePath(file_name);
}

}  // namespace

Logger& Logger::instance() {
  static Logger inst;
  return inst;
}

Logger::Logger() : writer_([this]() { writerLoop(); }) {}

Logger::~Logger() {
  {
    std::lock_guard<std::mutex> lk(queue_mutex_);
    cmds_.push_back(detail::CmdShutdown{});
  }
  queue_cv_.notify_one();
  if (writer_.joinable()) {
    writer_.join();
  }
}

pfd::base::TaskId Logger::globalTaskId() {
  return pfd::base::TaskId{};
}

void Logger::enqueueCmd(detail::AsyncCmd cmd) {
  std::lock_guard<std::mutex> lk(queue_mutex_);
  cmds_.push_back(std::move(cmd));
  queue_cv_.notify_one();
}

bool Logger::tryEnqueueLineToFile(const QString& line, const QString& path, bool flush_disk) {
  std::lock_guard<std::mutex> lk(queue_mutex_);
  if (cmds_.size() >= kMaxAsyncCmds) {
    dropped_async_cmds_.fetch_add(1, std::memory_order_relaxed);
    return false;
  }
  cmds_.push_back(detail::CmdLine{line, path, flush_disk});
  queue_cv_.notify_one();
  return true;
}

void Logger::setLogLevel(pfd::base::LogLevel level) {
  level_.store(level, std::memory_order_relaxed);
}

pfd::base::LogLevel Logger::logLevel() const {
  return level_.load(std::memory_order_relaxed);
}

void Logger::closeLogFileUnlocked() {
  if (log_file_) {
    log_file_->flush();
    log_file_->close();
    log_file_.reset();
  }
  log_file_path_.clear();
}

void Logger::reopenFileUnlocked(const QString& path) {
  closeLogFileUnlocked();
  log_file_path_ = path;
  if (path.isEmpty()) {
    return;
  }
  log_file_ = std::make_unique<QFile>(path);
  if (!log_file_->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    log_file_.reset();
    log_file_path_.clear();
  }
}

void Logger::writeTextLineUnlocked(const QString& line, bool flush_disk) {
  if (log_file_path_.isEmpty() || !log_file_ || !log_file_->isOpen()) {
    return;
  }
  QByteArray bytes = line.toUtf8();
  bytes += '\n';

  rotateIfNeededUnlocked(bytes.size());

  if (log_file_path_.isEmpty() || !log_file_ || !log_file_->isOpen()) {
    return;
  }

  if (log_file_->write(bytes) != bytes.size()) {
    return;
  }
  if (flush_disk) {
    log_file_->flush();
  }
}

void Logger::rotateIfNeededUnlocked(qint64 next_write_bytes) {
  const qint64 max_size = rotation_max_file_size_bytes_.load(std::memory_order_relaxed);
  const int backups = rotation_max_backup_files_.load(std::memory_order_relaxed);
  if (max_size <= 0 || backups <= 0) {
    return;
  }
  if (!log_file_ || !log_file_->isOpen()) {
    return;
  }
  const qint64 current_size = log_file_->size();
  if (current_size < 0) {
    return;
  }
  if (current_size + next_write_bytes <= max_size) {
    return;
  }
  rotateFilesUnlocked();
}

void Logger::rotateFilesUnlocked() {
  if (log_file_path_.isEmpty()) {
    return;
  }
  const int backups = rotation_max_backup_files_.load(std::memory_order_relaxed);
  if (backups <= 0) {
    return;
  }

  const QString base = log_file_path_;
  closeLogFileUnlocked();
  log_file_path_ = base;

  QFile::remove(QStringLiteral("%1.%2").arg(base).arg(backups));
  for (int i = backups - 1; i >= 1; --i) {
    const QString src = QStringLiteral("%1.%2").arg(base).arg(i);
    const QString dst = QStringLiteral("%1.%2").arg(base).arg(i + 1);
    if (QFile::exists(src)) {
      QFile::remove(dst);
      (void)QFile::rename(src, dst);
    }
  }
  if (QFile::exists(base)) {
    QFile::remove(base + QStringLiteral(".1"));
    (void)QFile::rename(base, base + QStringLiteral(".1"));
  }

  log_file_ = std::make_unique<QFile>(base);
  if (!log_file_->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    log_file_.reset();
    log_file_path_.clear();
  }
}

void Logger::processOneCmd(detail::AsyncCmd& cmd) {
  std::visit(
      [this](auto& c) {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, detail::CmdLine>) {
          std::lock_guard<std::mutex> fl(file_mutex_);
          if (!c.target_path.isEmpty() && c.target_path != log_file_path_) {
            reopenFileUnlocked(c.target_path);
          }
          writeTextLineUnlocked(c.text, c.flush_disk);
        } else if constexpr (std::is_same_v<T, detail::CmdReopen>) {
          std::lock_guard<std::mutex> fl(file_mutex_);
          reopenFileUnlocked(c.path);
        } else if constexpr (std::is_same_v<T, detail::CmdFlushSync>) {
          std::lock_guard<std::mutex> fl(file_mutex_);
          if (log_file_ && log_file_->isOpen()) {
            log_file_->flush();
          }
          c.done->set_value();
        } else if constexpr (std::is_same_v<T, detail::CmdBarrier>) {
          std::lock_guard<std::mutex> fl(file_mutex_);
          if (log_file_ && log_file_->isOpen()) {
            log_file_->flush();
          }
          c.done->set_value();
        } else if constexpr (std::is_same_v<T, detail::CmdShutdown>) {
          (void)c;  // 由 writerLoop 在 processOneCmd 之前处理
        }
      },
      cmd);
}

void Logger::writerLoop() {
  for (;;) {
    std::unique_lock<std::mutex> lk(queue_mutex_);
    queue_cv_.wait(lk, [this] { return !cmds_.empty(); });
    detail::AsyncCmd cmd = std::move(cmds_.front());
    cmds_.pop_front();
    lk.unlock();

    if (std::holds_alternative<detail::CmdShutdown>(cmd)) {
      std::lock_guard<std::mutex> fl(file_mutex_);
      closeLogFileUnlocked();
      return;
    }

    processOneCmd(cmd);
  }
}

void Logger::setLogFilePath(const QString& file_path) {
  QString opened_path;
  {
    std::lock_guard<std::mutex> lk(path_mutex_);
    configured_path_ = file_path;
    opened_path = datedLogPath(file_path);
  }
  if (!file_path.isEmpty()) {
    const QString dir = QFileInfo(file_path).absoluteDir().absolutePath();
    if (!dir.isEmpty()) {
      (void)pfd::base::ensureExists(dir);
    }
    file_output_enabled_.store(true, std::memory_order_relaxed);
  } else {
    file_output_enabled_.store(false, std::memory_order_relaxed);
  }
  enqueueCmd(detail::CmdReopen{opened_path});
}

QString Logger::logFilePath() const {
  std::lock_guard<std::mutex> lk(path_mutex_);
  return configured_path_;
}

void Logger::setFileOutputEnabled(bool enabled) {
  file_output_enabled_.store(enabled, std::memory_order_relaxed);
  if (!enabled) {
    enqueueCmd(detail::CmdReopen{QString()});
  } else {
    QString p;
    {
      std::lock_guard<std::mutex> lk(path_mutex_);
      p = datedLogPath(configured_path_);
    }
    if (!p.isEmpty()) {
      enqueueCmd(detail::CmdReopen{p});
    }
  }
}

bool Logger::isFileOutputEnabled() const {
  return file_output_enabled_.load(std::memory_order_relaxed);
}

uint64_t Logger::droppedAsyncCmdCount() const {
  return dropped_async_cmds_.load(std::memory_order_relaxed);
}

void Logger::setRotationPolicy(qint64 max_file_size_bytes, int max_backup_files) {
  rotation_max_file_size_bytes_.store(max_file_size_bytes, std::memory_order_relaxed);
  rotation_max_backup_files_.store(max_backup_files, std::memory_order_relaxed);
}

void Logger::flush() {
  auto p = std::make_shared<std::promise<void>>();
  std::future<void> f = p->get_future();
  enqueueCmd(detail::CmdFlushSync{std::move(p)});
  f.wait();
}

void Logger::waitUntilWriterIdle() {
  auto p = std::make_shared<std::promise<void>>();
  std::future<void> f = p->get_future();
  enqueueCmd(detail::CmdBarrier{std::move(p)});
  f.wait();
}

std::vector<pfd::core::LogEntry> Logger::globalSnapshot(std::size_t max_count) {
  return store_.snapshot(globalTaskId(), max_count);
}

void Logger::clearGlobalLogs() {
  store_.clear(globalTaskId());
}

void Logger::maybeEnqueueLineToFile(const pfd::core::LogEntry& entry) {
  if (!isFileOutputEnabled()) {
    return;
  }
  QString p;
  {
    std::lock_guard<std::mutex> lk(path_mutex_);
    p = datedLogPath(configured_path_);
  }
  if (p.isEmpty()) {
    return;
  }
  const QString line = formatLine(entry);
  const bool flush = entry.level >= pfd::base::LogLevel::kError;
  const bool queued = tryEnqueueLineToFile(line, p, flush);
  if (!queued && entry.level >= pfd::base::LogLevel::kError) {
    // 高等级日志在队列满时回退为同步落盘（仍在调用线程内尽力写）
    std::lock_guard<std::mutex> fl(file_mutex_);
    if (!p.isEmpty() && p != log_file_path_) {
      reopenFileUnlocked(p);
    }
    writeTextLineUnlocked(line, /*flush_disk=*/true);
  }
}

void Logger::log(pfd::base::LogLevel level, const QString& message, const char* file, int line,
                 const char* func) {
  const pfd::base::LogLevel current = logLevel();
  if (level < current) {
    return;
  }

  const QString detail = makeDetail(file, line, func);
  const auto entry = pfd::core::makeLogEntry(globalTaskId(), level, message, detail);

  store_.append(entry);
  maybeEnqueueLineToFile(entry);
}

void Logger::log(pfd::base::LogLevel level, const char* message, const char* file, int line,
                 const char* func) {
  log(level, message ? QString::fromUtf8(message) : QString(), file, line, func);
}

void Logger::logFatal(const QString& message, const char* file, int line, const char* func) {
  const QString detail = makeDetail(file, line, func);
  const auto entry =
      pfd::core::makeLogEntry(globalTaskId(), pfd::base::LogLevel::kFatal, message, detail);

  store_.append(entry);

  QString p;
  {
    std::lock_guard<std::mutex> lk(path_mutex_);
    p = datedLogPath(configured_path_);
  }

  if (!p.isEmpty()) {
    const QString line = formatLine(entry);
    const bool queued = tryEnqueueLineToFile(line, p, true);
    if (!queued) {
      std::lock_guard<std::mutex> fl(file_mutex_);
      if (p != log_file_path_) {
        reopenFileUnlocked(p);
      }
      writeTextLineUnlocked(line, /*flush_disk=*/true);
    }
    waitUntilWriterIdle();
  }

  std::quick_exit(1);
}

void Logger::logFatal(const char* message, const char* file, int line, const char* func) {
  logFatal(message ? QString::fromUtf8(message) : QString(), file, line, func);
}

QString Logger::formatLine(const pfd::core::LogEntry& entry) const {
  const QString iso = pfd::base::formatIso8601(entry.timestampMs);
  const QString lvl = pfd::base::logLevelToString(entry.level).toString();
  if (entry.detail.isEmpty()) {
    return QStringLiteral("[%1] [%2] %3").arg(iso, lvl, entry.message);
  }
  return QStringLiteral("[%1] [%2] %3 | %4").arg(iso, lvl, entry.message, entry.detail);
}

}  // namespace pfd::core::logger
