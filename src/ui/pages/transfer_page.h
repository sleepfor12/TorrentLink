#ifndef PFD_UI_PAGES_TRANSFER_PAGE_H
#define PFD_UI_PAGES_TRANSFER_PAGE_H

#include <QtCore/QSet>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <vector>

#include "base/types.h"
#include "core/task_snapshot.h"
#include "ui/pages/detail/content_tree_page.h"
#include "ui/pages/detail/http_source_page.h"
#include "ui/pages/detail/peer_list_page.h"
#include "ui/pages/detail/tracker_detail_page.h"

class QPushButton;
class QComboBox;
class QLineEdit;
class QVBoxLayout;
class QTableWidget;
class QLabel;
class QSplitter;

namespace pfd::ui {

class TaskDetailPanel;

class TransferPage final : public QWidget {
  Q_OBJECT

public:
  enum class TaskFilter : std::uint8_t {
    kAll = 0,
    kDownloading,
    kSeeding,
    kFinished,
    kChecking,
    kError,
    kRunning,
    kStopped,
    kActive,
    kIdle,
    kPaused,
    kUploadPaused,
    kDownloadPaused,
    kMoving,
    kErrorDetail,
  };

  enum class SortKey : std::uint8_t { kName = 0, kProgress, kDownloadRate, kStatus };
  enum class SortOrder : std::uint8_t { kDesc = 0, kAsc };

  explicit TransferPage(QWidget* parent = nullptr);

  void setSnapshots(const std::vector<pfd::core::TaskSnapshot>& snapshots);
  void addSpeedSample(qint64 downloadRate, qint64 uploadRate);
  void setContentHandlers(ContentTreePage::QueryFilesFn queryFn, ContentTreePage::SetPriorityFn priorityFn,
                          ContentTreePage::RenameFn renameFn);
  void setTrackerHandlers(TrackerDetailPage::QueryTrackersFn queryFn, TrackerDetailPage::AddTrackerFn addFn,
                          TrackerDetailPage::EditTrackerFn editFn, TrackerDetailPage::RemoveTrackerFn removeFn,
                          TrackerDetailPage::ReannounceTrackerFn reannounceFn,
                          TrackerDetailPage::ReannounceAllFn reannounceAllFn);
  void setPeerHandlers(PeerListPage::QueryPeersFn queryFn);
  void setHttpSourceHandlers(HttpSourcePage::QueryWebSeedsFn queryFn);
  void enterMemoryModeFromCurrentSelection();
  const std::vector<pfd::core::TaskSnapshot>& displayedSnapshots() const {
    return displayedSnapshots_;
  }

  [[nodiscard]] pfd::base::TaskId selectedTaskId() const;

  [[nodiscard]] TaskFilter currentFilter() const;
  void setFilter(TaskFilter f);

  [[nodiscard]] SortKey currentSortKey() const;
  void setSortKey(SortKey k);

  [[nodiscard]] SortOrder currentSortOrder() const;
  void setSortOrder(SortOrder o);

  [[nodiscard]] QByteArray taskHeaderSaveState() const;
  void restoreTaskHeaderState(const QByteArray& state);

  QTableWidget* taskTable() const {
    return taskTable_;
  }
  QComboBox* sortKeyBox() const {
    return sortKeyBox_;
  }
  QComboBox* sortOrderBox() const {
    return sortOrderBox_;
  }

Q_SIGNALS:
  void filterChanged();
  void sortChanged();
  void taskContextMenuRequested(const QPoint& pos);

private:
  void buildLayout();
  void bindSignals();
  void updateStats(const std::vector<pfd::core::TaskSnapshot>& snapshots);
  static bool matchFilter(const pfd::core::TaskSnapshot& snapshot, TaskFilter filter);

  QPushButton* filterAllBtn_{nullptr};
  QPushButton* filterDownloadingBtn_{nullptr};
  QPushButton* filterSeedingBtn_{nullptr};
  QPushButton* filterFinishedBtn_{nullptr};
  QPushButton* filterCheckingBtn_{nullptr};
  QPushButton* filterErrorBtn_{nullptr};
  QPushButton* filterRunningBtn_{nullptr};
  QPushButton* filterStoppedBtn_{nullptr};
  QPushButton* filterActiveBtn_{nullptr};
  QPushButton* filterIdleBtn_{nullptr};
  QPushButton* filterPausedBtn_{nullptr};
  QPushButton* filterUploadPausedBtn_{nullptr};
  QPushButton* filterDownloadPausedBtn_{nullptr};
  QPushButton* filterMovingBtn_{nullptr};
  QPushButton* filterErrorDetailBtn_{nullptr};

  QLineEdit* nameSearchEdit_{nullptr};
  QComboBox* sortKeyBox_{nullptr};
  QComboBox* sortOrderBox_{nullptr};
  QLabel* totalStatLabel_{nullptr};
  QLabel* downloadingStatLabel_{nullptr};
  QLabel* finishedStatLabel_{nullptr};
  QLabel* errorStatLabel_{nullptr};
  QTableWidget* taskTable_{nullptr};
  QSplitter* splitter_{nullptr};
  TaskDetailPanel* detailPanel_{nullptr};

  QVBoxLayout* tagFilterLayout_{nullptr};
  QList<QPushButton*> tagFilterButtons_;
  QString activeTagFilter_;
  QSet<QString> lastTagSet_;

  std::vector<pfd::core::TaskSnapshot> displayedSnapshots_;
  pfd::base::TaskId lastFocusedTaskId_;
  pfd::base::TaskId memoryTaskId_;
  bool isMemoryMode_{false};

  void updateDetailForSelection();
  int indexOfTask(const pfd::base::TaskId& id) const;
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_TRANSFER_PAGE_H
