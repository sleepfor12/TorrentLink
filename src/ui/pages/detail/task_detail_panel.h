#ifndef PFD_UI_PAGES_DETAIL_TASK_DETAIL_PANEL_H
#define PFD_UI_PAGES_DETAIL_TASK_DETAIL_PANEL_H

#include <QtWidgets/QWidget>

#include "core/task_snapshot.h"
#include "ui/pages/detail/content_tree_page.h"
#include "ui/pages/detail/http_source_page.h"
#include "ui/pages/detail/peer_list_page.h"
#include "ui/pages/detail/tracker_detail_page.h"

class QStackedWidget;
class QPushButton;

namespace pfd::ui {

class GeneralDetailPage;
class SpeedChartPage;
class TaskDetailPanel : public QWidget {
  Q_OBJECT

public:
  explicit TaskDetailPanel(QWidget* parent = nullptr);

  void setSnapshot(const pfd::core::TaskSnapshot& snap);
  void addSpeedSample(qint64 dlRate, qint64 ulRate);
  void setContentHandlers(ContentTreePage::QueryFilesFn queryFn, ContentTreePage::SetPriorityFn priorityFn,
                          ContentTreePage::RenameFn renameFn);
  void setTrackerHandlers(TrackerDetailPage::QueryTrackersFn queryFn, TrackerDetailPage::AddTrackerFn addFn,
                          TrackerDetailPage::EditTrackerFn editFn, TrackerDetailPage::RemoveTrackerFn removeFn,
                          TrackerDetailPage::ReannounceTrackerFn reannounceFn,
                          TrackerDetailPage::ReannounceAllFn reannounceAllFn);
  void setPeerHandlers(PeerListPage::QueryPeersFn queryFn);
  void setHttpSourceHandlers(HttpSourcePage::QueryWebSeedsFn queryFn);
  void clear();

private:
  void buildLayout();
  void switchTab(int index);

  QStackedWidget* stack_{nullptr};
  QPushButton* tabButtons_[6]{};
  int currentTab_{0};

  GeneralDetailPage* generalPage_{nullptr};
  TrackerDetailPage* trackerPage_{nullptr};
  PeerListPage* peersPage_{nullptr};
  HttpSourcePage* httpSourcePage_{nullptr};
  ContentTreePage* contentPage_{nullptr};
  SpeedChartPage* speedPage_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_TASK_DETAIL_PANEL_H
