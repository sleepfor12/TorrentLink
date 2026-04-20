#ifndef PFD_UI_PAGES_DETAIL_TRACKER_DETAIL_PAGE_H
#define PFD_UI_PAGES_DETAIL_TRACKER_DETAIL_PAGE_H

#include <QtWidgets/QWidget>

#include <functional>

#include "core/task_query_dto.h"
#include "core/task_snapshot.h"

class QTreeWidget;
class QTreeWidgetItem;
class QTimer;
class QShowEvent;
class QHideEvent;

namespace pfd::ui {

class TrackerDetailPage : public QWidget {
  Q_OBJECT

public:
  using QueryTrackersFn =
      std::function<pfd::core::TaskTrackerSnapshotDto(const pfd::base::TaskId&)>;
  using AddTrackerFn = std::function<void(const pfd::base::TaskId&, const QString&)>;
  using EditTrackerFn =
      std::function<void(const pfd::base::TaskId&, const QString&, const QString&)>;
  using RemoveTrackerFn = std::function<void(const pfd::base::TaskId&, const QString&)>;
  using ReannounceTrackerFn = std::function<void(const pfd::base::TaskId&, const QString&)>;
  using ReannounceAllFn = std::function<void(const pfd::base::TaskId&)>;

  explicit TrackerDetailPage(QWidget* parent = nullptr);
  void setHandlers(QueryTrackersFn queryFn, AddTrackerFn addFn, EditTrackerFn editFn,
                   RemoveTrackerFn removeFn, ReannounceTrackerFn reannounceFn,
                   ReannounceAllFn reannounceAllFn);
  void setSnapshot(const pfd::core::TaskSnapshot& snap);
  void clear();

protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

private:
  void buildLayout();
  void reload();
  void showContextMenu(const QPoint& pos);
  void actionAddTracker();
  void actionEditTracker(const QString& url);
  void actionRemoveTracker(const QString& url);
  void actionCopyTracker(const QString& url);
  void actionReannounceTracker(const QString& url);
  void actionReannounceAll();
  static QString statusText(pfd::core::TaskTrackerStatusDto s);
  static QString countText(int n);
  static QString announceText(int seconds);
  void decrementAnnounceCountdowns();
  void updateAnnounceCellsForItem(QTreeWidgetItem* urlRow);
  void applySavedSortIndicator();
  void pinFixedRowsToTop();

  QueryTrackersFn queryFn_;
  AddTrackerFn addFn_;
  EditTrackerFn editFn_;
  RemoveTrackerFn removeFn_;
  ReannounceTrackerFn reannounceFn_;
  ReannounceAllFn reannounceAllFn_;

  pfd::base::TaskId taskId_;
  QTreeWidget* tree_{nullptr};
  QTimer* refreshTimer_{nullptr};
  int refreshTick_{0};
  int sortColumn_{0};
  bool sortAscending_{true};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_TRACKER_DETAIL_PAGE_H
