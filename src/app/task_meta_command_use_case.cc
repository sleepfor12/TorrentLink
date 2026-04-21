#include "app/task_meta_command_use_case.h"

#include "base/input_sanitizer.h"

namespace pfd::app {

TaskMetaCommandUseCase::TaskMetaCommandUseCase(ConsumeEventFn consumeEventFn, LogFn logErrorFn)
    : consumeEventFn_(std::move(consumeEventFn)), logErrorFn_(std::move(logErrorFn)) {}

bool TaskMetaCommandUseCase::moveTask(const pfd::base::TaskId& taskId, const QString& targetPath) const {
  const auto pathErr = pfd::base::validatePath(targetPath);
  if (pathErr.hasError()) {
    if (logErrorFn_) {
      logErrorFn_(QStringLiteral("Move target path rejected: %1").arg(pathErr.message()));
    }
    return false;
  }
  return applyMeta(taskId, QString(), targetPath, QString(), QString());
}

bool TaskMetaCommandUseCase::renameTask(const pfd::base::TaskId& taskId, const QString& name) const {
  return applyMeta(taskId, name, QString(), QString(), QString());
}

bool TaskMetaCommandUseCase::updateCategory(const pfd::base::TaskId& taskId,
                                            const QString& category) const {
  const auto catErr = pfd::base::validateCategory(category);
  if (catErr.hasError()) {
    if (logErrorFn_) {
      logErrorFn_(QStringLiteral("Category rejected: %1").arg(catErr.message()));
    }
    return false;
  }
  return applyMeta(taskId, QString(), QString(), category, QString());
}

bool TaskMetaCommandUseCase::updateTags(const pfd::base::TaskId& taskId,
                                        const QStringList& tags) const {
  const QString csv = tags.join(QStringLiteral(","));
  const auto tagsErr = pfd::base::validateTagsCsv(csv);
  if (tagsErr.hasError()) {
    if (logErrorFn_) {
      logErrorFn_(QStringLiteral("Tags rejected: %1").arg(tagsErr.message()));
    }
    return false;
  }
  return applyMeta(taskId, QString(), QString(), QString(), csv);
}

bool TaskMetaCommandUseCase::upsertMeta(const pfd::base::TaskId& taskId, const QString& name,
                                        const QString& savePath, const QString& category,
                                        const QString& tagsCsv) const {
  return applyMeta(taskId, name, savePath, category, tagsCsv);
}

bool TaskMetaCommandUseCase::applyMeta(const pfd::base::TaskId& taskId, const QString& name,
                                       const QString& savePath, const QString& category,
                                       const QString& tagsCsv) const {
  if (!consumeEventFn_) {
    return false;
  }
  pfd::core::TaskEvent ev;
  ev.type = pfd::core::TaskEventType::kUpsertMeta;
  ev.taskId = taskId;
  ev.name = name;
  ev.savePath = savePath;
  ev.category = category;
  ev.tags = tagsCsv;
  consumeEventFn_(ev);
  return true;
}

}  // namespace pfd::app
