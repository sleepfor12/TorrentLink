#ifndef PFD_APP_TASK_META_COMMAND_USE_CASE_H
#define PFD_APP_TASK_META_COMMAND_USE_CASE_H

#include <QtCore/QString>

#include <functional>

#include "base/types.h"
#include "core/task_event.h"

namespace pfd::app {

class TaskMetaCommandUseCase {
public:
  using ConsumeEventFn = std::function<void(const pfd::core::TaskEvent&)>;
  using LogFn = std::function<void(const QString&)>;

  TaskMetaCommandUseCase(ConsumeEventFn consumeEventFn, LogFn logErrorFn);

  bool moveTask(const pfd::base::TaskId& taskId, const QString& targetPath) const;
  bool renameTask(const pfd::base::TaskId& taskId, const QString& name) const;
  bool updateCategory(const pfd::base::TaskId& taskId, const QString& category) const;
  bool updateTags(const pfd::base::TaskId& taskId, const QStringList& tags) const;
  bool upsertMeta(const pfd::base::TaskId& taskId, const QString& name, const QString& savePath,
                  const QString& category, const QString& tagsCsv) const;

private:
  bool applyMeta(const pfd::base::TaskId& taskId, const QString& name, const QString& savePath,
                 const QString& category, const QString& tagsCsv) const;

  ConsumeEventFn consumeEventFn_;
  LogFn logErrorFn_;
};

}  // namespace pfd::app

#endif  // PFD_APP_TASK_META_COMMAND_USE_CASE_H
