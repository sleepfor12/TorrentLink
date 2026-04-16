#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>

#include <gtest/gtest.h>

#include "core/save_path_policy.h"
#include "core/task_add_builder.h"

using pfd::core::buildTaskAdd;
using pfd::core::ContentLayout;
using pfd::core::SavePathPolicy;
using pfd::core::TaskAddInput;

class TaskAddBuilderTest : public ::testing::Test {
protected:
  void SetUp() override {
    ASSERT_TRUE(tmpDir_.isValid());
    policy_.setDefaultDownloadDir(tmpDir_.filePath("default"));
  }
  QTemporaryDir tmpDir_;
  SavePathPolicy policy_;
};

TEST_F(TaskAddBuilderTest, BasicOptionsMapping) {
  TaskAddInput in;
  in.name = "test_torrent";
  in.savePath = tmpDir_.filePath("explicit");
  in.layout = ContentLayout::kOriginal;
  in.startTorrent = false;
  in.stopCondition = 1;
  in.sequentialDownload = true;
  in.skipHashCheck = true;
  in.addToTopQueue = true;
  in.category = "movies";
  in.tagsCsv = "tag1,tag2";
  in.fileWanted = {1, 0, 1};

  const auto r = buildTaskAdd(in, policy_);

  EXPECT_EQ(r.finalSavePath, tmpDir_.filePath("explicit"));
  EXPECT_FALSE(r.opts.start_torrent);
  EXPECT_TRUE(r.opts.stop_when_ready);
  EXPECT_TRUE(r.opts.sequential_download);
  EXPECT_TRUE(r.opts.skip_hash_check);
  EXPECT_TRUE(r.opts.add_to_top_queue);
  EXPECT_EQ(r.opts.category, "movies");
  EXPECT_EQ(r.opts.tags_csv, "tag1,tag2");
  ASSERT_EQ(r.opts.file_priorities.size(), 3u);
  EXPECT_EQ(r.opts.file_priorities[0], 4u);
  EXPECT_EQ(r.opts.file_priorities[1], 0u);
  EXPECT_EQ(r.opts.file_priorities[2], 4u);
}

TEST_F(TaskAddBuilderTest, LayoutCreateSubfolder) {
  TaskAddInput in;
  in.name = "MyTorrent";
  in.savePath = tmpDir_.filePath("base");
  in.layout = ContentLayout::kCreateSubfolder;

  const auto r = buildTaskAdd(in, policy_);
  EXPECT_EQ(r.finalSavePath, QDir(tmpDir_.filePath("base")).filePath("MyTorrent"));
}

TEST_F(TaskAddBuilderTest, LayoutNoSubfolder) {
  TaskAddInput in;
  in.name = "MyTorrent";
  in.savePath = tmpDir_.filePath("base");
  in.layout = ContentLayout::kNoSubfolder;

  const auto r = buildTaskAdd(in, policy_);
  EXPECT_EQ(r.finalSavePath, tmpDir_.filePath("base"));
}

TEST_F(TaskAddBuilderTest, EmptySavePathUsesDefault) {
  TaskAddInput in;
  in.name = "test";
  in.layout = ContentLayout::kOriginal;

  const auto r = buildTaskAdd(in, policy_);
  EXPECT_EQ(r.finalSavePath, tmpDir_.filePath("default"));
}

TEST_F(TaskAddBuilderTest, StopConditionZero) {
  TaskAddInput in;
  in.stopCondition = 0;
  in.layout = ContentLayout::kOriginal;

  const auto r = buildTaskAdd(in, policy_);
  EXPECT_FALSE(r.opts.stop_when_ready);
}

TEST_F(TaskAddBuilderTest, EmptyFileWanted) {
  TaskAddInput in;
  in.layout = ContentLayout::kOriginal;

  const auto r = buildTaskAdd(in, policy_);
  EXPECT_TRUE(r.opts.file_priorities.empty());
}
