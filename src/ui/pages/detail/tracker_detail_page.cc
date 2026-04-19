#include "ui/pages/detail/tracker_detail_page.h"

#include <QtCore/QList>
#include <QtCore/QPoint>
#include <QtCore/QSet>
#include <QtCore/QSignalBlocker>
#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QHideEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QTreeWidgetItemIterator>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <limits>

#include "base/format.h"
#include "base/input_sanitizer.h"
#include "core/logger.h"

namespace pfd::ui {

namespace {
enum ItemType : int { kFixed = 0, kTracker = 1, kEndpoint = 2 };
constexpr int kColMsg = 7;
constexpr int kColNext = 8;
constexpr int kColMin = 9;
constexpr int kRoleType = Qt::UserRole + 1;
constexpr int kRoleUrl = Qt::UserRole + 2;
constexpr int kRoleNextAnnounce = Qt::UserRole + 3;
constexpr int kRoleMinAnnounce = Qt::UserRole + 4;
constexpr int kRoleSortUrl = Qt::UserRole + 10;
constexpr int kRoleSortTier = Qt::UserRole + 11;
QByteArray gTrackerHeaderState;
const QString kTierEmDash = QStringLiteral("\u2014");
constexpr int kTierSortNone = std::numeric_limits<int>::max();

[[nodiscard]] QString sortKeyForUrlColumn(const QString& displayUrl, const QString& roleUrl) {
  if (!roleUrl.isEmpty()) {
    return roleUrl.toLower();
  }
  return displayUrl.toLower();
}

[[nodiscard]] int parseCountForSort(const QString& t) {
  if (t == QStringLiteral("N/A") || t.isEmpty()) {
    return -1;
  }
  bool ok = false;
  const int v = t.toInt(&ok);
  return ok ? v : -1;
}

[[nodiscard]] qint64 announceSecsForSort(const QTreeWidgetItem& item, int announceCol) {
  const int role = announceCol == kColNext ? kRoleNextAnnounce : kRoleMinAnnounce;
  const QVariant v = item.data(announceCol, role);
  if (!v.isValid()) {
    return std::numeric_limits<qint64>::max();
  }
  const qint64 s = v.toLongLong();
  if (s < 0) {
    return std::numeric_limits<qint64>::max();
  }
  return s;
}

class TrackerTreeItem : public QTreeWidgetItem {
public:
  explicit TrackerTreeItem(QTreeWidget* parent) : QTreeWidgetItem(parent) {}
  explicit TrackerTreeItem(QTreeWidgetItem* parent) : QTreeWidgetItem(parent) {}

  bool operator<(const QTreeWidgetItem& other) const override {
    const QTreeWidget* tw = treeWidget();
    const int col = tw != nullptr ? tw->sortColumn() : 0;
    const QString ka = data(0, kRoleSortUrl).toString();
    const QString kb = other.data(0, kRoleSortUrl).toString();
    switch (col) {
      case 0:
        return QString::localeAwareCompare(ka, kb) < 0;
      case 1: {
        const int ta = data(0, kRoleSortTier).toInt();
        const int tb = other.data(0, kRoleSortTier).toInt();
        if (ta != tb) {
          return ta < tb;
        }
        return QString::localeAwareCompare(ka, kb) < 0;
      }
      case 2: {
        const QString pa = text(2);
        const QString pb = other.text(2);
        if (pa.isEmpty() != pb.isEmpty()) {
          return pa.isEmpty();
        }
        const int c = QString::localeAwareCompare(pa, pb);
        if (c != 0) {
          return c < 0;
        }
        return QString::localeAwareCompare(ka, kb) < 0;
      }
      case 3: {
        const int c = QString::localeAwareCompare(text(3), other.text(3));
        if (c != 0) {
          return c < 0;
        }
        return QString::localeAwareCompare(ka, kb) < 0;
      }
      case 4:
      case 5:
      case 6: {
        const int na = parseCountForSort(text(col));
        const int nb = parseCountForSort(other.text(col));
        if (na != nb) {
          return na < nb;
        }
        return QString::localeAwareCompare(ka, kb) < 0;
      }
      case 7: {
        const int c = QString::localeAwareCompare(text(kColMsg), other.text(kColMsg));
        if (c != 0) {
          return c < 0;
        }
        return QString::localeAwareCompare(ka, kb) < 0;
      }
      case 8:
      case 9: {
        const qint64 sa = announceSecsForSort(*this, col);
        const qint64 sb = announceSecsForSort(other, col);
        if (sa != sb) {
          return sa < sb;
        }
        return QString::localeAwareCompare(ka, kb) < 0;
      }
      default:
        return QString::localeAwareCompare(ka, kb) < 0;
    }
  }
};

[[nodiscard]] std::pair<int, int> minNextMinAmongChildren(QTreeWidgetItem* parent) {
  int minN = std::numeric_limits<int>::max();
  int minM = std::numeric_limits<int>::max();
  bool anyN = false;
  bool anyM = false;
  for (int i = 0; i < parent->childCount(); ++i) {
    QTreeWidgetItem* ch = parent->child(i);
    const int n = ch->data(kColNext, kRoleNextAnnounce).toInt();
    const int m = ch->data(kColMin, kRoleMinAnnounce).toInt();
    if (n >= 0) {
      minN = std::min(minN, n);
      anyN = true;
    }
    if (m >= 0) {
      minM = std::min(minM, m);
      anyM = true;
    }
  }
  return {anyN ? minN : -1, anyM ? minM : -1};
}

[[nodiscard]] QString qbStyleFixedLabel(const QString& rawName) {
  return QStringLiteral("** %1 **").arg(rawName.trimmed());
}

void applyTrackerItemLayout(QTreeWidgetItem* item) {
  if (item == nullptr) {
    return;
  }
  item->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
  item->setTextAlignment(2, Qt::AlignLeft | Qt::AlignVCenter);
  item->setTextAlignment(3, Qt::AlignLeft | Qt::AlignVCenter);
  item->setTextAlignment(kColMsg, Qt::AlignLeft | Qt::AlignVCenter);
  for (int col : {1, 4, 5, 6, kColNext, kColMin}) {
    item->setTextAlignment(col, Qt::AlignRight | Qt::AlignVCenter);
  }
}
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
  if (snap.taskId == taskId_) {
    return;
  }
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
  tree_->setColumnCount(10);
  tree_->setHeaderLabels({QStringLiteral("URL/Announce 端点"), QStringLiteral("层级"),
                          QStringLiteral("BT 协议"), QStringLiteral("状态"), QStringLiteral("用户"),
                          QStringLiteral("种子"), QStringLiteral("下载次数"),
                          QStringLiteral("消息"), QStringLiteral("下一个 Announce"),
                          QStringLiteral("最小 Announce")});
  tree_->setAlternatingRowColors(true);
  tree_->setAnimated(true);
  tree_->setUniformRowHeights(true);
  tree_->setIndentation(20);
  tree_->setRootIsDecorated(true);
  tree_->setContextMenuPolicy(Qt::CustomContextMenu);
  tree_->setStyleSheet(QStringLiteral(
      "QTreeWidget{border:1px solid #e7ebf3;border-radius:4px;background:#ffffff;"
      "alternate-background-color:#f6f8fc;outline:0;}"
      "QTreeWidget::item{padding:3px 6px;border:none;}"
      "QTreeWidget::item:selected,QTreeWidget::item:selected:active{"
      "background:#2f6fed;color:#ffffff;}"
      "QHeaderView::section{background:#f8fbff;color:#50607a;border:none;border-bottom:1px solid "
      "#e7ebf3;padding:8px;font-weight:600;}"));
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

  connect(tree_->header(), &QHeaderView::sortIndicatorChanged, this,
          [this](int logicalIndex, Qt::SortOrder order) {
            sortColumn_ = logicalIndex;
            sortAscending_ = (order == Qt::AscendingOrder);
            QTimer::singleShot(0, this, [this]() { pinFixedRowsToTop(); });
          });
  connect(tree_, &QTreeWidget::itemExpanded, this,
          [this](QTreeWidgetItem* it) { updateAnnounceCellsForItem(it); });
  connect(tree_, &QTreeWidget::itemCollapsed, this,
          [this](QTreeWidgetItem* it) { updateAnnounceCellsForItem(it); });
  tree_->setSortingEnabled(true);
  tree_->header()->setSortIndicator(0, Qt::AscendingOrder);

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
  if (tree_->header() != nullptr) {
    sortColumn_ = tree_->header()->sortIndicatorSection();
    sortAscending_ = (tree_->header()->sortIndicatorOrder() == Qt::AscendingOrder);
  }
  QScrollBar* vbar = tree_->verticalScrollBar();
  const int scrollValue = vbar != nullptr ? vbar->value() : 0;
  tree_->setSortingEnabled(false);
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
    auto* item = new TrackerTreeItem(tree_);
    const QString display0 = type == kFixed ? qbStyleFixedLabel(r.url) : r.url;
    item->setText(0, display0);
    item->setData(0, kRoleSortUrl, sortKeyForUrlColumn(display0, r.url));
    item->setData(0, kRoleType, type);
    item->setData(0, kRoleUrl, r.url);
    if (type == kFixed) {
      item->setText(1, kTierEmDash);
      item->setData(0, kRoleSortTier, -1);
      item->setText(2, QString());
    } else if (type == kTracker) {
      item->setText(1, QString::number(r.tier));
      item->setData(0, kRoleSortTier, r.tier);
      item->setText(2, QString());
    }
    item->setText(3, statusText(r.status));
    item->setText(4, countText(r.users));
    item->setText(5, countText(r.seeds));
    item->setText(6, countText(r.downloads));
    item->setText(kColMsg, r.message);
    if (type == kFixed) {
      item->setForeground(0, QBrush(QColor(71, 85, 105)));
      item->setForeground(3, QBrush(QColor(100, 116, 139)));
    } else {
      item->setForeground(3, QBrush(statusColor(r.status)));
    }
    applyTrackerItemLayout(item);
    const bool expanded = expandedUrls.contains(r.url);
    for (const auto& ep : r.endpoints) {
      auto* child = new TrackerTreeItem(item);
      const QString epLabel = QStringLiteral("%1:%2").arg(ep.ip).arg(ep.port);
      child->setText(0, epLabel);
      child->setData(0, kRoleSortUrl, epLabel.toLower());
      child->setText(1, QString());
      child->setData(0, kRoleSortTier, kTierSortNone);
      child->setText(2, ep.btProtocol.isEmpty() ? QStringLiteral("N/A") : ep.btProtocol);
      child->setText(3, statusText(ep.status));
      child->setText(4, countText(ep.users));
      child->setText(5, countText(ep.seeds));
      child->setText(6, countText(ep.downloads));
      child->setText(kColMsg, ep.message);
      child->setText(kColNext, announceText(ep.nextAnnounceSec));
      child->setText(kColMin, announceText(ep.minAnnounceSec));
      child->setData(0, kRoleType, kEndpoint);
      child->setData(0, kRoleUrl, r.url);
      child->setData(kColNext, kRoleNextAnnounce, ep.nextAnnounceSec);
      child->setData(kColMin, kRoleMinAnnounce, ep.minAnnounceSec);
      child->setForeground(3, QBrush(statusColor(ep.status)));
      applyTrackerItemLayout(child);
      if (toSelect == nullptr && selectedType == kEndpoint && selectedUrl == r.url &&
          selectedEndpointText == child->text(0) && expanded) {
        toSelect = child;
      }
    }
    if (item->childCount() > 0) {
      const auto [mn, mm] = minNextMinAmongChildren(item);
      item->setData(kColNext, kRoleNextAnnounce, mn);
      item->setData(kColMin, kRoleMinAnnounce, mm);
      if (expanded) {
        item->setText(kColNext, QString());
        item->setText(kColMin, QString());
      } else {
        item->setText(kColNext, mn >= 0 ? announceText(mn) : QStringLiteral("N/A"));
        item->setText(kColMin, mm >= 0 ? announceText(mm) : QStringLiteral("N/A"));
      }
    } else {
      item->setData(kColNext, kRoleNextAnnounce, r.nextAnnounceSec);
      item->setData(kColMin, kRoleMinAnnounce, r.minAnnounceSec);
      item->setText(kColNext, announceText(r.nextAnnounceSec));
      item->setText(kColMin, announceText(r.minAnnounceSec));
    }
    if (expanded) {
      item->setExpanded(true);
    }
    if (toSelect == nullptr && selectedType == kEndpoint && selectedUrl == r.url && !expanded) {
      toSelect = item;
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
  if (vbar != nullptr) {
    vbar->setValue(std::min(scrollValue, vbar->maximum()));
  }
  tree_->setSortingEnabled(true);
  applySavedSortIndicator();
}

void TrackerDetailPage::applySavedSortIndicator() {
  if (!tree_ || tree_->header() == nullptr)
    return;
  if (sortColumn_ < 0 || sortColumn_ >= tree_->columnCount()) {
    sortColumn_ = 0;
  }
  const Qt::SortOrder ord = sortAscending_ ? Qt::AscendingOrder : Qt::DescendingOrder;
  tree_->header()->setSortIndicator(sortColumn_, ord);
  tree_->sortByColumn(sortColumn_, ord);
  pinFixedRowsToTop();
}

void TrackerDetailPage::pinFixedRowsToTop() {
  if (!tree_)
    return;
  QList<QTreeWidgetItem*> fixed;
  QList<QTreeWidgetItem*> trackers;
  fixed.reserve(3);
  const int n = tree_->topLevelItemCount();
  for (int i = 0; i < n; ++i) {
    QTreeWidgetItem* it = tree_->topLevelItem(i);
    if (it->data(0, kRoleType).toInt() == kFixed) {
      fixed.append(it);
    } else {
      trackers.append(it);
    }
  }
  if (fixed.isEmpty()) {
    return;
  }
  static const QStringList kFixedOrder = {QStringLiteral("[DHT]"), QStringLiteral("[PeX]"),
                                          QStringLiteral("[LSD]")};
  QList<QTreeWidgetItem*> fixedOrdered;
  fixedOrdered.reserve(fixed.size());
  for (const QString& key : kFixedOrder) {
    for (QTreeWidgetItem* f : fixed) {
      if (f->data(0, kRoleUrl).toString() == key) {
        fixedOrdered.append(f);
        break;
      }
    }
  }
  for (QTreeWidgetItem* f : fixed) {
    bool seen = false;
    for (QTreeWidgetItem* o : fixedOrdered) {
      if (o == f) {
        seen = true;
        break;
      }
    }
    if (!seen) {
      fixedOrdered.append(f);
    }
  }
  const Qt::SortOrder ord = sortAscending_ ? Qt::AscendingOrder : Qt::DescendingOrder;
  {
    QSignalBlocker blocker(tree_->header());
    tree_->setSortingEnabled(false);
    while (tree_->topLevelItemCount() > 0) {
      tree_->takeTopLevelItem(0);
    }
    for (QTreeWidgetItem* it : fixedOrdered) {
      tree_->addTopLevelItem(it);
    }
    for (QTreeWidgetItem* it : trackers) {
      tree_->addTopLevelItem(it);
    }
    tree_->setSortingEnabled(true);
    if (tree_->header() != nullptr) {
      tree_->header()->setSortIndicator(sortColumn_, ord);
    }
  }
}

void TrackerDetailPage::updateAnnounceCellsForItem(QTreeWidgetItem* urlRow) {
  if (!tree_ || urlRow == nullptr)
    return;
  const int t = urlRow->data(0, kRoleType).toInt();
  if (t != kTracker && t != kFixed)
    return;
  if (urlRow->childCount() == 0)
    return;
  const auto [mn, mm] = minNextMinAmongChildren(urlRow);
  urlRow->setData(kColNext, kRoleNextAnnounce, mn);
  urlRow->setData(kColMin, kRoleMinAnnounce, mm);
  if (urlRow->isExpanded()) {
    urlRow->setText(kColNext, QString());
    urlRow->setText(kColMin, QString());
  } else {
    urlRow->setText(kColNext, mn >= 0 ? announceText(mn) : QStringLiteral("N/A"));
    urlRow->setText(kColMin, mm >= 0 ? announceText(mm) : QStringLiteral("N/A"));
  }
}

void TrackerDetailPage::decrementAnnounceCountdowns() {
  if (!tree_)
    return;
  for (QTreeWidgetItemIterator it(tree_); *it != nullptr; ++it) {
    auto* item = *it;
    const int typ = item->data(0, kRoleType).toInt();
    if (typ != kEndpoint && item->childCount() > 0)
      continue;
    int nextSec = item->data(kColNext, kRoleNextAnnounce).toInt();
    int minSec = item->data(kColMin, kRoleMinAnnounce).toInt();
    if (nextSec > 0)
      --nextSec;
    if (minSec > 0)
      --minSec;
    item->setData(kColNext, kRoleNextAnnounce, nextSec);
    item->setData(kColMin, kRoleMinAnnounce, minSec);
    item->setText(kColNext, announceText(nextSec));
    item->setText(kColMin, announceText(minSec));
  }
  for (int i = 0; i < tree_->topLevelItemCount(); ++i) {
    QTreeWidgetItem* tl = tree_->topLevelItem(i);
    if (tl->childCount() == 0)
      continue;
    const auto [mn, mm] = minNextMinAmongChildren(tl);
    tl->setData(kColNext, kRoleNextAnnounce, mn);
    tl->setData(kColMin, kRoleMinAnnounce, mm);
    updateAnnounceCellsForItem(tl);
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
