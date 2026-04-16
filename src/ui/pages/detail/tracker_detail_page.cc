#include "ui/pages/detail/tracker_detail_page.h"

#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QHideEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QTreeWidgetItemIterator>
#include <QtWidgets/QVBoxLayout>

#include "base/format.h"
#include "base/input_sanitizer.h"
#include "core/logger.h"

namespace pfd::ui {

namespace {
enum ItemType : int { kFixed = 0, kTracker = 1, kEndpoint = 2 };
constexpr int kRoleType = Qt::UserRole + 1;
constexpr int kRoleUrl = Qt::UserRole + 2;
constexpr int kRoleNextAnnounce = Qt::UserRole + 3;
constexpr int kRoleMinAnnounce = Qt::UserRole + 4;
QByteArray gTrackerHeaderState;
}  // namespace

TrackerDetailPage::TrackerDetailPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void TrackerDetailPage::setHandlers(QueryTrackersFn queryFn, AddTrackerFn addFn,
                                    EditTrackerFn editFn, RemoveTrackerFn removeFn,
                                    ReannounceTrackerFn reannounceFn,
                                    ReannounceAllFn reannounceAllFn) {
  queryFn_ = std::move(queryFn);
  addFn_ = std::move(addFn);
  editFn_ = std::move(editFn);
  removeFn_ = std::move(removeFn);
  reannounceFn_ = std::move(reannounceFn);
  reannounceAllFn_ = std::move(reannounceAllFn);
}

void TrackerDetailPage::setSnapshot(const pfd::core::TaskSnapshot& snap) {
  taskId_ = snap.taskId;
  reload();
}

void TrackerDetailPage::clear() {
  taskId_ = {};
  if (tree_)
    tree_->clear();
}

void TrackerDetailPage::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  if (refreshTimer_ != nullptr && !refreshTimer_->isActive()) {
    refreshTimer_->start();
  }
  if (!taskId_.isNull()) {
    reload();
  }
}

void TrackerDetailPage::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
  if (refreshTimer_ != nullptr) {
    refreshTimer_->stop();
  }
}

void TrackerDetailPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(6, 6, 6, 6);
  root->setSpacing(6);

  tree_ = new QTreeWidget(this);
  tree_->setColumnCount(9);
  tree_->setHeaderLabels({QStringLiteral("URL/Announce 端点"), QStringLiteral("层级"),
                          QStringLiteral("BT 协议"), QStringLiteral("状态"), QStringLiteral("用户"),
                          QStringLiteral("种子"), QStringLiteral("下载"),
                          QStringLiteral("下一个 Announce"), QStringLiteral("最小 Announce")});
  tree_->setAlternatingRowColors(true);
  tree_->setContextMenuPolicy(Qt::CustomContextMenu);
  tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  if (!gTrackerHeaderState.isEmpty()) {
    tree_->header()->restoreState(gTrackerHeaderState);
  }
  connect(tree_->header(), &QHeaderView::sectionResized, this, [this](int, int, int) {
    if (tree_ != nullptr && tree_->header() != nullptr) {
      gTrackerHeaderState = tree_->header()->saveState();
    }
  });
  root->addWidget(tree_, 1);
  connect(tree_, &QTreeWidget::customContextMenuRequested, this,
          [this](const QPoint& pos) { showContextMenu(pos); });

  refreshTimer_ = new QTimer(this);
  refreshTimer_->setInterval(1000);
  connect(refreshTimer_, &QTimer::timeout, this, [this]() {
    if (!isVisible() || taskId_.isNull())
      return;
    decrementAnnounceCountdowns();
    ++refreshTick_;
    if (refreshTick_ >= 3) {
      refreshTick_ = 0;
      reload();
    }
  });
}

QString TrackerDetailPage::statusText(pfd::lt::SessionWorker::TrackerStatus s) {
  using T = pfd::lt::SessionWorker::TrackerStatus;
  switch (s) {
    case T::kCannotConnect:
      return QStringLiteral("无法连接");
    case T::kWorking:
      return QStringLiteral("工作");
    case T::kUpdating:
      return QStringLiteral("更新中");
    case T::kNotWorking:
    default:
      return QStringLiteral("未工作");
  }
}

QString TrackerDetailPage::countText(int n) {
  return n >= 0 ? QString::number(n) : QStringLiteral("N/A");
}

QString TrackerDetailPage::announceText(int seconds) {
  if (seconds < 0)
    return QStringLiteral("N/A");
  return pfd::base::formatDuration(seconds);
}

static QColor statusColor(pfd::lt::SessionWorker::TrackerStatus s) {
  using T = pfd::lt::SessionWorker::TrackerStatus;
  switch (s) {
    case T::kWorking:
      return QColor(22, 163, 74);
    case T::kUpdating:
      return QColor(37, 99, 235);
    case T::kCannotConnect:
      return QColor(220, 38, 38);
    case T::kNotWorking:
    default:
      return QColor(100, 116, 139);
  }
}

void TrackerDetailPage::reload() {
  if (!tree_)
    return;
  QSet<QString> expandedUrls;
  QString selectedUrl;
  int selectedType = -1;
  QString selectedEndpointText;
  if (QTreeWidgetItem* current = tree_->currentItem(); current != nullptr) {
    selectedUrl = current->data(0, kRoleUrl).toString();
    selectedType = current->data(0, kRoleType).toInt();
    if (selectedType == kEndpoint) {
      selectedEndpointText = current->text(0);
    }
  }
  for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
    QTreeWidgetItem* oldItem = tree_->topLevelItem(i);
    if (oldItem != nullptr && oldItem->isExpanded()) {
      expandedUrls.insert(oldItem->data(0, kRoleUrl).toString());
    }
  }
  tree_->clear();
  if (taskId_.isNull() || !queryFn_)
    return;
  const auto data = queryFn_(taskId_);
  LOG_DEBUG(QStringLiteral("[ui.tracker] reload taskId=%1 fixed=%2 trackers=%3")
                .arg(taskId_.toString())
                .arg(data.fixedRows.size())
                .arg(data.trackers.size()));

  QTreeWidgetItem* toSelect = nullptr;
  const auto appendRow = [this, &expandedUrls, &selectedUrl, &selectedType, &selectedEndpointText,
                          &toSelect](const pfd::lt::SessionWorker::TrackerRowSnapshot& r,
                                     int type) {
    auto* item = new QTreeWidgetItem(tree_);
    item->setText(0, r.url);
    item->setText(1, type == kTracker ? QString::number(r.tier) : QStringLiteral("0"));
    item->setText(2, r.btProtocol.isEmpty() ? QStringLiteral("N/A") : r.btProtocol);
    item->setText(3, statusText(r.status));
    item->setText(4, countText(r.users));
    item->setText(5, countText(r.seeds));
    item->setText(6, countText(r.downloads));
    item->setText(7, announceText(r.nextAnnounceSec));
    item->setText(8, announceText(r.minAnnounceSec));
    item->setData(0, kRoleType, type);
    item->setData(0, kRoleUrl, r.url);
    item->setData(7, kRoleNextAnnounce, r.nextAnnounceSec);
    item->setData(8, kRoleMinAnnounce, r.minAnnounceSec);
    if (type == kFixed) {
      item->setForeground(0, QBrush(QColor(100, 116, 139)));
      item->setForeground(3, QBrush(QColor(100, 116, 139)));
    } else {
      item->setForeground(3, QBrush(statusColor(r.status)));
    }
    for (const auto& ep : r.endpoints) {
      auto* child = new QTreeWidgetItem(item);
      child->setText(0, QStringLiteral("%1:%2").arg(ep.ip).arg(ep.port));
      child->setText(1, QString());
      child->setText(2, ep.btProtocol.isEmpty() ? QStringLiteral("N/A") : ep.btProtocol);
      child->setText(3, statusText(ep.status));
      child->setText(4, countText(ep.users));
      child->setText(5, countText(ep.seeds));
      child->setText(6, countText(ep.downloads));
      child->setText(7, announceText(ep.nextAnnounceSec));
      child->setText(8, announceText(ep.minAnnounceSec));
      child->setData(0, kRoleType, kEndpoint);
      child->setData(0, kRoleUrl, r.url);
      child->setForeground(3, QBrush(statusColor(ep.status)));
      if (toSelect == nullptr && selectedType == kEndpoint && selectedUrl == r.url &&
          selectedEndpointText == child->text(0)) {
        toSelect = child;
      }
    }
    if (expandedUrls.contains(r.url)) {
      item->setExpanded(true);
    }
    if (toSelect == nullptr && selectedType != kEndpoint && selectedUrl == r.url) {
      toSelect = item;
    }
    return item;
  };

  for (const auto& row : data.fixedRows)
    appendRow(row, kFixed);
  for (const auto& row : data.trackers)
    appendRow(row, kTracker);
  if (toSelect != nullptr) {
    tree_->setCurrentItem(toSelect);
  }
}

void TrackerDetailPage::decrementAnnounceCountdowns() {
  if (!tree_)
    return;
  for (QTreeWidgetItemIterator it(tree_); *it != nullptr; ++it) {
    auto* item = *it;
    const int type = item->data(0, kRoleType).toInt();
    if (type != kFixed && type != kTracker)
      continue;
    int nextSec = item->data(7, kRoleNextAnnounce).toInt();
    int minSec = item->data(8, kRoleMinAnnounce).toInt();
    if (nextSec > 0)
      --nextSec;
    if (minSec > 0)
      --minSec;
    item->setData(7, kRoleNextAnnounce, nextSec);
    item->setData(8, kRoleMinAnnounce, minSec);
    item->setText(7, announceText(nextSec));
    item->setText(8, announceText(minSec));
  }
}

void TrackerDetailPage::showContextMenu(const QPoint& pos) {
  if (!tree_ || taskId_.isNull())
    return;
  QTreeWidgetItem* item = tree_->itemAt(pos);
  const int t = item ? item->data(0, kRoleType).toInt() : -1;
  const QString url = item ? item->data(0, kRoleUrl).toString() : QString();

  QMenu menu(this);
  auto* addAct = menu.addAction(QStringLiteral("添加 Tracker"));
  QAction* editAct = nullptr;
  QAction* copyAct = nullptr;
  QAction* removeAct = nullptr;
  QAction* reannounceAct = nullptr;
  QAction* reannounceAllAct = nullptr;

  if (item != nullptr && t != kFixed) {
    editAct = menu.addAction(QStringLiteral("编辑 Tracker URL"));
    copyAct = menu.addAction(QStringLiteral("复制 Tracker URL"));
    removeAct = menu.addAction(QStringLiteral("移除 Tracker"));
    reannounceAct = menu.addAction(QStringLiteral("强制向该 Tracker 重新汇报"));
  }
  reannounceAllAct = menu.addAction(QStringLiteral("强制向所有 Tracker 重新汇报"));
  QAction* chosen = menu.exec(tree_->viewport()->mapToGlobal(pos));
  if (!chosen)
    return;
  if (chosen == addAct)
    return actionAddTracker();
  if (chosen == editAct)
    return actionEditTracker(url);
  if (chosen == copyAct)
    return actionCopyTracker(url);
  if (chosen == removeAct)
    return actionRemoveTracker(url);
  if (chosen == reannounceAct)
    return actionReannounceTracker(url);
  if (chosen == reannounceAllAct)
    return actionReannounceAll();
}

void TrackerDetailPage::actionAddTracker() {
  if (!addFn_)
    return;
  bool ok = false;
  const QString url =
      QInputDialog::getText(this, QStringLiteral("添加 Tracker"), QStringLiteral("Tracker URL"),
                            QLineEdit::Normal, QString(), &ok)
          .trimmed();
  if (!ok || url.isEmpty())
    return;
  const auto err = pfd::base::validateTrackerUrl(url);
  if (err.hasError()) {
    LOG_WARN(QStringLiteral("[ui.tracker] add tracker rejected taskId=%1 reason=%2")
                 .arg(taskId_.toString(), err.message()));
    return;
  }
  addFn_(taskId_, url);
  LOG_INFO(
      QStringLiteral("[ui.tracker] add tracker taskId=%1 url=%2").arg(taskId_.toString(), url));
  reload();
}

void TrackerDetailPage::actionEditTracker(const QString& url) {
  if (!editFn_ || url.trimmed().isEmpty())
    return;
  bool ok = false;
  const QString newUrl =
      QInputDialog::getText(this, QStringLiteral("编辑 Tracker URL"), QStringLiteral("Tracker URL"),
                            QLineEdit::Normal, url, &ok)
          .trimmed();
  if (!ok || newUrl.isEmpty() || newUrl == url)
    return;
  const auto err = pfd::base::validateTrackerUrl(newUrl);
  if (err.hasError()) {
    LOG_WARN(QStringLiteral("[ui.tracker] edit tracker rejected taskId=%1 old=%2 reason=%3")
                 .arg(taskId_.toString(), url, err.message()));
    return;
  }
  editFn_(taskId_, url, newUrl);
  LOG_INFO(QStringLiteral("[ui.tracker] edit tracker taskId=%1 old=%2 new=%3")
               .arg(taskId_.toString(), url, newUrl));
  reload();
}

void TrackerDetailPage::actionRemoveTracker(const QString& url) {
  if (!removeFn_ || url.trimmed().isEmpty())
    return;
  removeFn_(taskId_, url);
  LOG_INFO(
      QStringLiteral("[ui.tracker] remove tracker taskId=%1 url=%2").arg(taskId_.toString(), url));
  reload();
}

void TrackerDetailPage::actionCopyTracker(const QString& url) {
  if (url.trimmed().isEmpty())
    return;
  if (auto* cb = QApplication::clipboard(); cb)
    cb->setText(url);
}

void TrackerDetailPage::actionReannounceTracker(const QString& url) {
  if (!reannounceFn_ || url.trimmed().isEmpty())
    return;
  reannounceFn_(taskId_, url);
  LOG_INFO(QStringLiteral("[ui.tracker] reannounce tracker taskId=%1 url=%2")
               .arg(taskId_.toString(), url));
}

void TrackerDetailPage::actionReannounceAll() {
  if (!reannounceAllFn_)
    return;
  reannounceAllFn_(taskId_);
  LOG_INFO(
      QStringLiteral("[ui.tracker] reannounce all trackers taskId=%1").arg(taskId_.toString()));
}

}  // namespace pfd::ui
