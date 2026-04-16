#ifndef PFD_CORE_LOGGER_H
#define PFD_CORE_LOGGER_H

#include <QtCore/QString>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>
#include <vector>

#include "core/log_store.h"

class QFile;

namespace pfd::core::logger {

namespace detail {

struct CmdLine {
  QString text;
  QString target_path;
  bool flush_disk{false};
};

struct CmdReopen {
  QString path;
};

struct CmdFlushSync {
  std::shared_ptr<std::promise<void>> done;
};

struct CmdBarrier {
  std::shared_ptr<std::promise<void>> done;
};

struct CmdShutdown {};

using AsyncCmd = std::variant<CmdLine, CmdReopen, CmdFlushSync, CmdBarrier, CmdShutdown>;

}  // namespace detail

class Logger {
public:
  static Logger& instance();

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  void setLogLevel(pfd::base::LogLevel level);
  pfd::base::LogLevel logLevel() const;

  /// @brief 设置日志落盘文件路径；并启用文件输出。
  ///        传空路径则关闭文件输出；实际开关文件在异步写盘线程中完成。
  void setLogFilePath(const QString& file_path);
  QString logFilePath() const;

  void setFileOutputEnabled(bool enabled);
  bool isFileOutputEnabled() const;

  /// @brief 设置日志轮转策略：
  /// - max_file_size_bytes: 单文件最大字节数（<=0 表示关闭轮转）
  /// - max_backup_files: 归档文件数量上限（如 3 -> .1/.2/.3）
  /// 超限时覆盖最旧归档。
  void setRotationPolicy(qint64 max_file_size_bytes, int max_backup_files);

  /// @brief 等待队列中先于本次调用的条目写完并 flush 磁盘（异步写盘语义下的“刷盘”）
  void flush();

  [[nodiscard]] std::vector<pfd::core::LogEntry> globalSnapshot(std::size_t max_count);

  void clearGlobalLogs();
  [[nodiscard]] uint64_t droppedAsyncCmdCount() const;

  void log(pfd::base::LogLevel level, const QString& message, const char* file, int line,
           const char* func);
  void log(pfd::base::LogLevel level, const char* message, const char* file, int line,
           const char* func);

  /// @brief FATAL：不受日志级别过滤；排空异步队列并 flush 后 quick_exit(1)
  void logFatal(const QString& message, const char* file, int line, const char* func);
  void logFatal(const char* message, const char* file, int line, const char* func);

private:
  Logger();
  ~Logger();

  static pfd::base::TaskId globalTaskId();

  void enqueueCmd(detail::AsyncCmd cmd);
  void maybeEnqueueLineToFile(const pfd::core::LogEntry& entry);
  bool tryEnqueueLineToFile(const QString& line, const QString& path, bool flush_disk);
  void waitUntilWriterIdle();

  void writerLoop();
  void processOneCmd(detail::AsyncCmd& cmd);
  void reopenFileUnlocked(const QString& path);
  void closeLogFileUnlocked();
  void rotateIfNeededUnlocked(qint64 next_write_bytes);
  void rotateFilesUnlocked();
  void writeTextLineUnlocked(const QString& line, bool flush_disk);

  QString formatLine(const pfd::core::LogEntry& entry) const;

  // --- async queue ---
  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::deque<detail::AsyncCmd> cmds_;
  std::thread writer_;
  static constexpr size_t kMaxAsyncCmds = 8192;
  std::atomic<uint64_t> dropped_async_cmds_{0};

  // --- file (仅写盘线程在持有 file_mutex_ 时访问 QFile) ---
  mutable std::mutex file_mutex_;
  QString log_file_path_;
  std::unique_ptr<QFile> log_file_;

  mutable std::mutex path_mutex_;
  QString configured_path_;

  std::atomic<bool> file_output_enabled_{false};
  std::atomic<qint64> rotation_max_file_size_bytes_{0};
  std::atomic<int> rotation_max_backup_files_{3};
  std::atomic<pfd::base::LogLevel> level_{pfd::base::LogLevel::kInfo};
  pfd::core::LogStore store_;
};

}  // namespace pfd::core::logger

#define PFD_LOGGER_REF_ (::pfd::core::logger::Logger::instance())

#if defined(NDEBUG)
#  define LOG_DEBUG(MSG) ((void)0)
#else
#  define LOG_DEBUG(MSG)                                                                \
    do {                                                                                \
      auto& _pfd_l = PFD_LOGGER_REF_;                                                   \
      if (_pfd_l.logLevel() <= ::pfd::base::LogLevel::kDebug) {                         \
        _pfd_l.log(::pfd::base::LogLevel::kDebug, (MSG), __FILE__, __LINE__, __func__); \
      }                                                                                 \
    } while (0)
#endif

#define LOG_INFO(MSG)                                                                \
  do {                                                                               \
    auto& _pfd_l = PFD_LOGGER_REF_;                                                  \
    if (_pfd_l.logLevel() <= ::pfd::base::LogLevel::kInfo) {                         \
      _pfd_l.log(::pfd::base::LogLevel::kInfo, (MSG), __FILE__, __LINE__, __func__); \
    }                                                                                \
  } while (0)

#define LOG_WARN(MSG)                                                                   \
  do {                                                                                  \
    auto& _pfd_l = PFD_LOGGER_REF_;                                                     \
    if (_pfd_l.logLevel() <= ::pfd::base::LogLevel::kWarning) {                         \
      _pfd_l.log(::pfd::base::LogLevel::kWarning, (MSG), __FILE__, __LINE__, __func__); \
    }                                                                                   \
  } while (0)

#define LOG_ERROR(MSG)                                                                \
  do {                                                                                \
    auto& _pfd_l = PFD_LOGGER_REF_;                                                   \
    if (_pfd_l.logLevel() <= ::pfd::base::LogLevel::kError) {                         \
      _pfd_l.log(::pfd::base::LogLevel::kError, (MSG), __FILE__, __LINE__, __func__); \
    }                                                                                 \
  } while (0)

/// 请直接使用本宏：记录 FATAL、排空异步写盘队列并 flush 后立刻退出进程。
#define LOG_FATAL(MSG)                                             \
  do {                                                             \
    PFD_LOGGER_REF_.logFatal((MSG), __FILE__, __LINE__, __func__); \
  } while (0)

#endif  // PFD_CORE_LOGGER_H
