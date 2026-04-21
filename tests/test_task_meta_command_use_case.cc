#include <QtCore/QStringList>
#include <QtCore/QUuid>

#include <gtest/gtest.h>

#include "app/task_meta_command_use_case.h"

namespace {

TEST(TaskMetaCommandUseCase, ValidatesAndEmitsEvents) {
  int consumeCount = 0;
  int errorCount = 0;
  pfd::core::TaskEvent lastEvent;
  QString lastError;

  pfd::app::TaskMetaCommandUseCase useCase(
      [&](const pfd::core::TaskEvent& ev) {
        ++consumeCount;
        lastEvent = ev;
      },
      [&](const QString& err) {
        ++errorCount;
        lastError = err;
      });

  const auto taskId = QUuid::createUuid();
  EXPECT_TRUE(useCase.renameTask(taskId, QStringLiteral("name")));
  EXPECT_EQ(consumeCount, 1);
  EXPECT_EQ(lastEvent.type, pfd::core::TaskEventType::kUpsertMeta);
  EXPECT_EQ(lastEvent.name, QStringLiteral("name"));

  EXPECT_TRUE(useCase.moveTask(taskId, QStringLiteral("/tmp")));
  EXPECT_EQ(consumeCount, 2);
  EXPECT_EQ(lastEvent.savePath, QStringLiteral("/tmp"));

  EXPECT_TRUE(useCase.updateCategory(taskId, QStringLiteral("cat")));
  EXPECT_EQ(consumeCount, 3);
  EXPECT_EQ(lastEvent.category, QStringLiteral("cat"));

  EXPECT_TRUE(useCase.updateTags(taskId, QStringList{QStringLiteral("t1"), QStringLiteral("t2")}));
  EXPECT_EQ(consumeCount, 4);
  EXPECT_EQ(lastEvent.tags, QStringLiteral("t1,t2"));

  EXPECT_FALSE(useCase.moveTask(taskId, QStringLiteral("")));
  EXPECT_TRUE(useCase.updateCategory(taskId, QStringLiteral("/bad")));
  EXPECT_TRUE(useCase.updateTags(taskId, QStringList{QStringLiteral("a,b")}));
  EXPECT_EQ(consumeCount, 6);
  EXPECT_EQ(errorCount, 1);
  EXPECT_FALSE(lastError.isEmpty());
}

}  // namespace
