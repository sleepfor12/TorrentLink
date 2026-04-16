#include "ui/pages/detail/task_detail_panel.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>

#include <utility>

#include "ui/pages/detail/content_tree_page.h"
#include "ui/pages/detail/general_detail_page.h"
#include "ui/pages/detail/http_source_page.h"
#include "ui/pages/detail/peer_list_page.h"
#include "ui/pages/detail/speed_chart_page.h"
#include "ui/pages/detail/tracker_detail_page.h"

namespace pfd::ui {

TaskDetailPanel::TaskDetailPanel(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void TaskDetailPanel::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  stack_ = new QStackedWidget(this);

  generalPage_ = new GeneralDetailPage(stack_);
  trackerPage_ = new TrackerDetailPage(stack_);
  peersPage_ = new PeerListPage(stack_);
  httpSourcePage_ = new HttpSourcePage(stack_);
  contentPage_ = new ContentTreePage(stack_);
  speedPage_ = new SpeedChartPage(stack_);

  stack_->addWidget(generalPage_);
  stack_->addWidget(trackerPage_);
  stack_->addWidget(peersPage_);
  stack_->addWidget(httpSourcePage_);
  stack_->addWidget(contentPage_);
  stack_->addWidget(speedPage_);

  root->addWidget(stack_, 1);

  // Tab bar
  auto* tabBar = new QWidget(this);
  tabBar->setObjectName(QStringLiteral("DetailTabBar"));
  tabBar->setStyleSheet(
      QStringLiteral("#DetailTabBar{background:#f9fafb;border-top:1px solid #e5e7eb;}"));
  auto* tabLayout = new QHBoxLayout(tabBar);
  tabLayout->setContentsMargins(8, 4, 8, 4);
  tabLayout->setSpacing(4);

  const QStringList labels = {QStringLiteral("普通"), QStringLiteral("Tracker"),
                              QStringLiteral("用户"), QStringLiteral("HTTP源"),
                              QStringLiteral("内容"), QStringLiteral("速度")};
  const QStringList icons = {QStringLiteral("📋"), QStringLiteral("📡"), QStringLiteral("👥"),
                             QStringLiteral("🌐"), QStringLiteral("📁"), QStringLiteral("📈")};

  for (int i = 0; i < 6; ++i) {
    tabButtons_[i] = new QPushButton(icons[i] + QStringLiteral(" ") + labels[i], tabBar);
    tabButtons_[i]->setCheckable(true);
    tabButtons_[i]->setFlat(true);
    tabButtons_[i]->setStyleSheet(QStringLiteral(
        "QPushButton{padding:4px 10px;border-radius:4px;font-size:12px;color:#4b5563;}"
        "QPushButton:checked{background:#e0e7ff;color:#3730a3;font-weight:700;}"
        "QPushButton:hover{background:#f3f4f6;}"));
    tabLayout->addWidget(tabButtons_[i]);
    connect(tabButtons_[i], &QPushButton::clicked, this, [this, i]() { switchTab(i); });
  }
  tabLayout->addStretch(1);
  root->addWidget(tabBar);

  switchTab(0);
}

void TaskDetailPanel::switchTab(int index) {
  if (index < 0 || index > 5)
    return;
  currentTab_ = index;
  stack_->setCurrentIndex(index);
  for (int i = 0; i < 6; ++i) {
    tabButtons_[i]->setChecked(i == index);
  }
}

void TaskDetailPanel::setSnapshot(const pfd::core::TaskSnapshot& snap) {
  generalPage_->setSnapshot(snap);
  trackerPage_->setSnapshot(snap);
  peersPage_->setSnapshot(snap);
  httpSourcePage_->setSnapshot(snap);
  contentPage_->setSnapshot(snap);
}

void TaskDetailPanel::addSpeedSample(qint64 dlRate, qint64 ulRate) {
  speedPage_->addSample(dlRate, ulRate);
}

void TaskDetailPanel::setContentHandlers(ContentTreePage::QueryFilesFn queryFn,
                                         ContentTreePage::SetPriorityFn priorityFn,
                                         ContentTreePage::RenameFn renameFn) {
  contentPage_->setHandlers(std::move(queryFn), std::move(priorityFn), std::move(renameFn));
}

void TaskDetailPanel::setTrackerHandlers(TrackerDetailPage::QueryTrackersFn queryFn,
                                         TrackerDetailPage::AddTrackerFn addFn,
                                         TrackerDetailPage::EditTrackerFn editFn,
                                         TrackerDetailPage::RemoveTrackerFn removeFn,
                                         TrackerDetailPage::ReannounceTrackerFn reannounceFn,
                                         TrackerDetailPage::ReannounceAllFn reannounceAllFn) {
  trackerPage_->setHandlers(std::move(queryFn), std::move(addFn), std::move(editFn),
                            std::move(removeFn), std::move(reannounceFn),
                            std::move(reannounceAllFn));
}

void TaskDetailPanel::setPeerHandlers(PeerListPage::QueryPeersFn queryFn) {
  peersPage_->setQueryFn(std::move(queryFn));
}

void TaskDetailPanel::setHttpSourceHandlers(HttpSourcePage::QueryWebSeedsFn queryFn) {
  httpSourcePage_->setQueryFn(std::move(queryFn));
}

void TaskDetailPanel::clear() {
  generalPage_->clear();
  trackerPage_->clear();
  peersPage_->clear();
  httpSourcePage_->clear();
  contentPage_->clear();
}

}  // namespace pfd::ui
