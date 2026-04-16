#include "ui/pages/transfer_page.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QSignalBlocker>
#include <QtCore/QUuid>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QVBoxLayout>

#include <utility>

#include "base/format.h"
#include "ui/pages/detail/task_detail_panel.h"

namespace pfd::ui {

namespace {

QString statusToText(pfd::base::TaskStatus status) {
  using S = pfd::base::TaskStatus;
  switch (status) {
    case S::kQueued:
      return QStringLiteral("等待中");
    case S::kChecking:
      return QStringLiteral("校验中");
    case S::kDownloading:
      return QStringLiteral("下载中");
    case S::kSeeding:
      return QStringLiteral("做种中");
    case S::kPaused:
      return QStringLiteral("已暂停");
    case S::kFinished:
      return QStringLiteral("已完成");
    case S::kError:
      return QStringLiteral("错误");
    case S::kUnknown:
    default:
      return QStringLiteral("未知");
  }
}

QString estimateEtaText(const pfd::core::TaskSnapshot& s) {
  if (s.totalBytes <= 0 || s.downloadRate <= 0) {
    return QStringLiteral("--");
  }
  const qint64 left = std::max<qint64>(0, s.totalBytes - s.downloadedBytes);
  if (left <= 0) {
    return QStringLiteral("0s");
  }
  return pfd::base::formatDuration(left / s.downloadRate);
}

QString ratioText(const pfd::core::TaskSnapshot& s) {
  if (s.downloadedBytes <= 0) {
    return QStringLiteral("--");
  }
  return pfd::base::formatRatio(static_cast<double>(s.uploadedBytes) /
                                static_cast<double>(s.downloadedBytes));
}

}  // namespace

TransferPage::TransferPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
  bindSignals();
}

void TransferPage::buildLayout() {
  auto* root = new QHBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 16);
  root->setSpacing(12);

  auto* sideBar = new QWidget(this);
  sideBar->setObjectName(QStringLiteral("SideBar"));
  sideBar->setFixedWidth(170);
  auto* sideLayout = new QVBoxLayout(sideBar);
  sideLayout->setContentsMargins(12, 12, 12, 12);
  sideLayout->setSpacing(8);
  auto* sideTitle = new QLabel(QStringLiteral("状态列表"), sideBar);
  sideTitle->setStyleSheet(QStringLiteral("font-size:14px;font-weight:700;color:#1f2d3d;"));
  sideLayout->addWidget(sideTitle);

  filterAllBtn_ = new QPushButton(QStringLiteral("全部"), sideBar);
  filterDownloadingBtn_ = new QPushButton(QStringLiteral("下载"), sideBar);
  filterSeedingBtn_ = new QPushButton(QStringLiteral("做种"), sideBar);
  filterFinishedBtn_ = new QPushButton(QStringLiteral("完成"), sideBar);
  filterErrorBtn_ = new QPushButton(QStringLiteral("错误"), sideBar);
  filterRunningBtn_ = new QPushButton(QStringLiteral("正运行"), sideBar);
  filterStoppedBtn_ = new QPushButton(QStringLiteral("已停止"), sideBar);
  filterActiveBtn_ = new QPushButton(QStringLiteral("活动"), sideBar);
  filterIdleBtn_ = new QPushButton(QStringLiteral("空闲"), sideBar);
  filterPausedBtn_ = new QPushButton(QStringLiteral("暂停"), sideBar);
  filterUploadPausedBtn_ = new QPushButton(QStringLiteral("上传已暂停"), sideBar);
  filterDownloadPausedBtn_ = new QPushButton(QStringLiteral("下载已暂停"), sideBar);
  filterCheckingBtn_ = new QPushButton(QStringLiteral("正在检查"), sideBar);
  filterMovingBtn_ = new QPushButton(QStringLiteral("正在移动"), sideBar);
  filterErrorDetailBtn_ = new QPushButton(QStringLiteral("错误"), sideBar);
  for (auto* btn :
       {filterAllBtn_, filterDownloadingBtn_, filterSeedingBtn_, filterFinishedBtn_,
        filterErrorBtn_, filterRunningBtn_, filterStoppedBtn_, filterActiveBtn_, filterIdleBtn_,
        filterPausedBtn_, filterUploadPausedBtn_, filterDownloadPausedBtn_, filterCheckingBtn_,
        filterMovingBtn_, filterErrorDetailBtn_}) {
    btn->setObjectName(QStringLiteral("FilterButton"));
    btn->setCheckable(true);
    sideLayout->addWidget(btn);
  }
  filterAllBtn_->setChecked(true);

  auto* tagTitle = new QLabel(QStringLiteral("标签"), sideBar);
  tagTitle->setStyleSheet(
      QStringLiteral("font-size:14px;font-weight:700;color:#1f2d3d;margin-top:8px;"));
  sideLayout->addWidget(tagTitle);
  tagFilterLayout_ = new QVBoxLayout();
  tagFilterLayout_->setSpacing(4);
  sideLayout->addLayout(tagFilterLayout_);

  sideLayout->addStretch(1);
  root->addWidget(sideBar);

  auto* mainPanel = new QWidget(this);
  auto* v = new QVBoxLayout(mainPanel);
  v->setContentsMargins(0, 0, 0, 0);
  v->setSpacing(12);

  auto* title = new QLabel(QStringLiteral("下载任务"), mainPanel);
  title->setStyleSheet(QStringLiteral("font-size:22px;font-weight:700;color:#1f2d3d;"));
  v->addWidget(title);

  auto* statRow = new QHBoxLayout();
  statRow->setSpacing(8);
  const auto makeCard = [mainPanel](const QString& title, QLabel** outValue) {
    auto* card = new QWidget(mainPanel);
    card->setObjectName(QStringLiteral("StatCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    auto* t = new QLabel(title, card);
    t->setObjectName(QStringLiteral("StatTitle"));
    auto* vlabel = new QLabel(QStringLiteral("0"), card);
    vlabel->setObjectName(QStringLiteral("StatValue"));
    layout->addWidget(t);
    layout->addWidget(vlabel);
    *outValue = vlabel;
    return card;
  };
  statRow->addWidget(makeCard(QStringLiteral("总任务"), &totalStatLabel_));
  statRow->addWidget(makeCard(QStringLiteral("下载中"), &downloadingStatLabel_));
  statRow->addWidget(makeCard(QStringLiteral("已完成"), &finishedStatLabel_));
  statRow->addWidget(makeCard(QStringLiteral("错误"), &errorStatLabel_));
  v->addLayout(statRow);

  nameSearchEdit_ = new QLineEdit(mainPanel);
  nameSearchEdit_->setPlaceholderText(QStringLiteral("按任务名搜索..."));
  nameSearchEdit_->setClearButtonEnabled(true);
  v->addWidget(nameSearchEdit_);

  auto* sortRow = new QHBoxLayout();
  sortRow->setSpacing(8);
  auto* sortLabel = new QLabel(QStringLiteral("排序"), mainPanel);
  sortKeyBox_ = new QComboBox(mainPanel);
  sortOrderBox_ = new QComboBox(mainPanel);
  sortKeyBox_->addItems(QStringList{QStringLiteral("按名称"), QStringLiteral("按进度"),
                                    QStringLiteral("按下载速度"), QStringLiteral("按状态")});
  sortOrderBox_->addItems(QStringList{QStringLiteral("降序"), QStringLiteral("升序")});
  sortKeyBox_->setCurrentIndex(2);
  sortRow->addWidget(sortLabel);
  sortRow->addWidget(sortKeyBox_);
  sortRow->addWidget(sortOrderBox_);
  sortRow->addStretch(1);
  v->addLayout(sortRow);

  taskTable_ = new QTableWidget(mainPanel);
  taskTable_->setColumnCount(15);
  taskTable_->setHorizontalHeaderLabels(
      {QStringLiteral("名称"), QStringLiteral("状态"), QStringLiteral("进度"),
       QStringLiteral("已下载"), QStringLiteral("下载速度"), QStringLiteral("上传速度"),
       QStringLiteral("选定大小"), QStringLiteral("做种数"), QStringLiteral("用户"),
       QStringLiteral("剩余时间"), QStringLiteral("比率"), QStringLiteral("分类"),
       QStringLiteral("标签"), QStringLiteral("可用性"), QStringLiteral("错误信息")});
  taskTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  taskTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  taskTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  taskTable_->setContextMenuPolicy(Qt::CustomContextMenu);
  taskTable_->setAlternatingRowColors(true);
  taskTable_->verticalHeader()->setVisible(false);
  taskTable_->horizontalHeader()->setStretchLastSection(true);
  taskTable_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  detailPanel_ = new TaskDetailPanel(mainPanel);

  splitter_ = new QSplitter(Qt::Vertical, mainPanel);
  splitter_->addWidget(taskTable_);
  splitter_->addWidget(detailPanel_);
  splitter_->setStretchFactor(0, 3);
  splitter_->setStretchFactor(1, 2);
  splitter_->setChildrenCollapsible(false);
  v->addWidget(splitter_, 2);

  root->addWidget(mainPanel, 1);
}

void TransferPage::bindSignals() {
  const auto activateOnly = [this](QPushButton* current) {
    for (auto* btn :
         {filterAllBtn_, filterDownloadingBtn_, filterSeedingBtn_, filterFinishedBtn_,
          filterErrorBtn_, filterRunningBtn_, filterStoppedBtn_, filterActiveBtn_, filterIdleBtn_,
          filterPausedBtn_, filterUploadPausedBtn_, filterDownloadPausedBtn_, filterCheckingBtn_,
          filterMovingBtn_, filterErrorDetailBtn_}) {
      if (btn != nullptr) {
        btn->setChecked(btn == current);
      }
    }
    emit filterChanged();
  };
  connect(filterAllBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterAllBtn_); });
  connect(filterDownloadingBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterDownloadingBtn_); });
  connect(filterSeedingBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterSeedingBtn_); });
  connect(filterFinishedBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterFinishedBtn_); });
  connect(filterErrorBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterErrorBtn_); });
  connect(filterRunningBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterRunningBtn_); });
  connect(filterStoppedBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterStoppedBtn_); });
  connect(filterActiveBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterActiveBtn_); });
  connect(filterIdleBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterIdleBtn_); });
  connect(filterPausedBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterPausedBtn_); });
  connect(filterUploadPausedBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterUploadPausedBtn_); });
  connect(filterDownloadPausedBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterDownloadPausedBtn_); });
  connect(filterCheckingBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterCheckingBtn_); });
  connect(filterMovingBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterMovingBtn_); });
  connect(filterErrorDetailBtn_, &QPushButton::clicked, this,
          [activateOnly, this]() { activateOnly(filterErrorDetailBtn_); });

  connect(nameSearchEdit_, &QLineEdit::textChanged, this,
          [this](const QString&) { emit filterChanged(); });

  connect(sortKeyBox_, &QComboBox::currentIndexChanged, this, [this](int) { emit sortChanged(); });
  connect(sortOrderBox_, &QComboBox::currentIndexChanged, this,
          [this](int) { emit sortChanged(); });

  if (taskTable_ != nullptr) {
    connect(taskTable_, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { emit taskContextMenuRequested(pos); });
    connect(taskTable_, &QTableWidget::currentCellChanged, this, [this](int row, int, int, int) {
      if (row >= 0 && row < static_cast<int>(displayedSnapshots_.size())) {
        lastFocusedTaskId_ = displayedSnapshots_[static_cast<std::size_t>(row)].taskId;
        isMemoryMode_ = false;
      }
      updateDetailForSelection();
    });
  }
}

void TransferPage::setSnapshots(const std::vector<pfd::core::TaskSnapshot>& snapshots) {
  if (taskTable_ == nullptr) {
    displayedSnapshots_.clear();
    updateStats(snapshots);
    return;
  }

  const pfd::base::TaskId previousSelected = selectedTaskId();
  QSet<QString> previousSelectedIds;
  if (taskTable_ != nullptr && taskTable_->selectionModel() != nullptr) {
    const auto rows = taskTable_->selectionModel()->selectedRows();
    for (const auto& idx : rows) {
      const int r = idx.row();
      if (r >= 0 && r < static_cast<int>(displayedSnapshots_.size())) {
        previousSelectedIds.insert(
            displayedSnapshots_[static_cast<std::size_t>(r)].taskId.toString(QUuid::WithoutBraces));
      }
    }
  }

  updateStats(snapshots);
  const auto filter = currentFilter();
  const QString nameFilter =
      nameSearchEdit_ != nullptr ? nameSearchEdit_->text().trimmed() : QString();

  std::vector<pfd::core::TaskSnapshot> visible;
  visible.reserve(snapshots.size());
  for (const auto& s : snapshots) {
    if (!matchFilter(s, filter))
      continue;
    if (!nameFilter.isEmpty() && !s.name.contains(nameFilter, Qt::CaseInsensitive))
      continue;
    if (!activeTagFilter_.isEmpty() && !s.tags.contains(activeTagFilter_, Qt::CaseInsensitive))
      continue;
    visible.push_back(s);
  }

  const auto order = currentSortOrder();
  const auto lessByOrder = [&](auto&& a, auto&& b) {
    return order == SortOrder::kAsc ? (a < b) : (a > b);
  };
  switch (currentSortKey()) {
    case SortKey::kName:
      std::sort(visible.begin(), visible.end(), [&](const auto& a, const auto& b) {
        return lessByOrder(a.name.toLower(), b.name.toLower());
      });
      break;
    case SortKey::kProgress:
      std::sort(visible.begin(), visible.end(), [&](const auto& a, const auto& b) {
        return lessByOrder(a.progress01(), b.progress01());
      });
      break;
    case SortKey::kStatus:
      std::sort(visible.begin(), visible.end(), [&](const auto& a, const auto& b) {
        return lessByOrder(static_cast<int>(a.status), static_cast<int>(b.status));
      });
      break;
    case SortKey::kDownloadRate:
    default:
      std::sort(visible.begin(), visible.end(), [&](const auto& a, const auto& b) {
        return lessByOrder(a.downloadRate, b.downloadRate);
      });
      break;
  }

  displayedSnapshots_ = visible;

  taskTable_->setUpdatesEnabled(false);
  const QSignalBlocker blocker(taskTable_);

  const int oldRowCount = taskTable_->rowCount();
  const int newRowCount = static_cast<int>(visible.size());

  if (newRowCount > oldRowCount) {
    for (int r = oldRowCount; r < newRowCount; ++r) {
      taskTable_->insertRow(r);
    }
  } else if (newRowCount < oldRowCount) {
    for (int r = oldRowCount - 1; r >= newRowCount; --r) {
      taskTable_->removeRow(r);
    }
  }

  int restoreRow = -1;
  QList<int> restoreRows;

  const auto rowColorForStatus = [](pfd::base::TaskStatus status) -> QColor {
    if (status == pfd::base::TaskStatus::kDownloading)
      return {40, 167, 69};
    if (status == pfd::base::TaskStatus::kPaused)
      return {128, 128, 128};
    if (status == pfd::base::TaskStatus::kFinished || status == pfd::base::TaskStatus::kSeeding ||
        status == pfd::base::TaskStatus::kError)
      return {220, 53, 69};
    return {31, 41, 55};
  };

  const auto statusColorForStatus = [](pfd::base::TaskStatus status) -> QColor {
    if (status == pfd::base::TaskStatus::kError)
      return {220, 53, 69};
    if (status == pfd::base::TaskStatus::kDownloading)
      return {40, 167, 69};
    if (status == pfd::base::TaskStatus::kPaused)
      return {128, 128, 128};
    if (status == pfd::base::TaskStatus::kFinished || status == pfd::base::TaskStatus::kSeeding)
      return {220, 53, 69};
    return {31, 41, 55};
  };

  const auto ensureItem = [&](int r, int col, const QString& text, const QColor& color) {
    auto* existing = taskTable_->item(r, col);
    if (existing != nullptr) {
      existing->setText(text);
      existing->setForeground(QBrush(color));
    } else {
      auto* item = new QTableWidgetItem(text);
      item->setForeground(QBrush(color));
      taskTable_->setItem(r, col, item);
    }
  };

  for (int row = 0; row < newRowCount; ++row) {
    const auto& s = visible[static_cast<std::size_t>(row)];

    if (!previousSelected.isNull() && s.taskId == previousSelected) {
      restoreRow = row;
    }
    if (previousSelectedIds.contains(s.taskId.toString(QUuid::WithoutBraces))) {
      restoreRows.push_back(row);
    }

    QColor rowColor = rowColorForStatus(s.status);
    const bool isMemoryRow = isMemoryMode_ && !memoryTaskId_.isNull() && s.taskId == memoryTaskId_;
    if (isMemoryRow) {
      rowColor = QColor(148, 163, 184);
    }

    QString displayName = s.name.trimmed();
    if (displayName.isEmpty()) {
      displayName = QStringLiteral("任务-%1").arg(s.taskId.toString(QUuid::WithoutBraces).left(8));
    }
    ensureItem(row, 0, displayName, rowColor);
    ensureItem(row, 1, statusToText(s.status), statusColorForStatus(s.status));

    const int progressVal = static_cast<int>(s.progress01() * 1000.0);
    const auto progressText = QStringLiteral("%1%").arg(s.progress01() * 100.0, 0, 'f', 1);
    auto* bar = qobject_cast<QProgressBar*>(taskTable_->cellWidget(row, 2));
    if (bar != nullptr) {
      bar->setValue(progressVal);
      bar->setFormat(progressText);
      bar->setStyleSheet(QStringLiteral("QProgressBar{color:%1;}").arg(rowColor.name()));
    } else {
      bar = new QProgressBar(taskTable_);
      bar->setRange(0, 1000);
      bar->setValue(progressVal);
      bar->setFormat(progressText);
      bar->setTextVisible(true);
      bar->setStyleSheet(QStringLiteral("QProgressBar{color:%1;}").arg(rowColor.name()));
      bar->setFocusPolicy(Qt::NoFocus);
      bar->setAttribute(Qt::WA_TransparentForMouseEvents, true);
      taskTable_->setCellWidget(row, 2, bar);
    }

    ensureItem(row, 3, pfd::base::formatBytes(s.downloadedBytes), rowColor);
    ensureItem(row, 4, pfd::base::formatRate(s.downloadRate), rowColor);
    ensureItem(row, 5, pfd::base::formatRate(s.uploadRate), rowColor);
    ensureItem(row, 6, pfd::base::formatBytes(s.selectedBytes > 0 ? s.selectedBytes : s.totalBytes),
               rowColor);
    ensureItem(row, 7, QString::number(s.seeders), rowColor);
    ensureItem(row, 8, QString::number(s.users), rowColor);
    ensureItem(row, 9, estimateEtaText(s), rowColor);
    ensureItem(row, 10, ratioText(s), rowColor);
    ensureItem(row, 11, s.category.isEmpty() ? QStringLiteral("Default") : s.category, rowColor);
    ensureItem(row, 12, s.tags.isEmpty() ? QStringLiteral("--") : s.tags, rowColor);
    ensureItem(row, 13,
               s.availability > 0.0 ? QStringLiteral("%1").arg(s.availability, 0, 'f', 2)
                                    : QStringLiteral("--"),
               rowColor);
    ensureItem(row, 14, s.errorMessage, rowColor);
  }

  if (isMemoryMode_) {
    taskTable_->clearSelection();
    taskTable_->setCurrentCell(-1, -1);
  } else {
    int targetRow = -1;
    if (!lastFocusedTaskId_.isNull()) {
      targetRow = indexOfTask(lastFocusedTaskId_);
    }
    if (targetRow < 0 && restoreRow >= 0 && restoreRow < taskTable_->rowCount()) {
      targetRow = restoreRow;
    }
    if (!restoreRows.isEmpty() && taskTable_->selectionModel() != nullptr) {
      taskTable_->clearSelection();
      for (int r : restoreRows) {
        if (r < 0 || r >= taskTable_->rowCount())
          continue;
        const QModelIndex idx = taskTable_->model()->index(r, 0);
        taskTable_->selectionModel()->select(idx, QItemSelectionModel::Select |
                                                      QItemSelectionModel::Rows);
      }
      if (targetRow >= 0 && targetRow < taskTable_->rowCount()) {
        const QModelIndex curIdx = taskTable_->model()->index(targetRow, 0);
        taskTable_->selectionModel()->setCurrentIndex(curIdx, QItemSelectionModel::NoUpdate);
      }
    } else if (targetRow >= 0 && targetRow < taskTable_->rowCount()) {
      taskTable_->setCurrentCell(targetRow, 0);
    } else {
      taskTable_->clearSelection();
      taskTable_->setCurrentCell(-1, -1);
    }
  }
  taskTable_->setUpdatesEnabled(true);

  updateDetailForSelection();
}

void TransferPage::addSpeedSample(qint64 downloadRate, qint64 uploadRate) {
  if (detailPanel_ == nullptr)
    return;
  detailPanel_->addSpeedSample(downloadRate, uploadRate);
}

void TransferPage::setContentHandlers(ContentTreePage::QueryFilesFn queryFn,
                                      ContentTreePage::SetPriorityFn priorityFn,
                                      ContentTreePage::RenameFn renameFn) {
  if (detailPanel_ == nullptr)
    return;
  detailPanel_->setContentHandlers(std::move(queryFn), std::move(priorityFn), std::move(renameFn));
}

void TransferPage::setTrackerHandlers(TrackerDetailPage::QueryTrackersFn queryFn,
                                      TrackerDetailPage::AddTrackerFn addFn,
                                      TrackerDetailPage::EditTrackerFn editFn,
                                      TrackerDetailPage::RemoveTrackerFn removeFn,
                                      TrackerDetailPage::ReannounceTrackerFn reannounceFn,
                                      TrackerDetailPage::ReannounceAllFn reannounceAllFn) {
  if (detailPanel_ == nullptr)
    return;
  detailPanel_->setTrackerHandlers(std::move(queryFn), std::move(addFn), std::move(editFn),
                                   std::move(removeFn), std::move(reannounceFn),
                                   std::move(reannounceAllFn));
}

void TransferPage::setPeerHandlers(PeerListPage::QueryPeersFn queryFn) {
  if (detailPanel_ == nullptr)
    return;
  detailPanel_->setPeerHandlers(std::move(queryFn));
}

void TransferPage::setHttpSourceHandlers(HttpSourcePage::QueryWebSeedsFn queryFn) {
  if (detailPanel_ == nullptr)
    return;
  detailPanel_->setHttpSourceHandlers(std::move(queryFn));
}

void TransferPage::enterMemoryModeFromCurrentSelection() {
  if (taskTable_ == nullptr)
    return;
  const int row = taskTable_->currentRow();
  if (row < 0 || row >= static_cast<int>(displayedSnapshots_.size()))
    return;
  memoryTaskId_ = displayedSnapshots_[static_cast<std::size_t>(row)].taskId;
  lastFocusedTaskId_ = memoryTaskId_;
  isMemoryMode_ = true;
  taskTable_->clearSelection();
  taskTable_->setCurrentCell(-1, -1);
  updateDetailForSelection();
  taskTable_->viewport()->update();
}

pfd::base::TaskId TransferPage::selectedTaskId() const {
  if (taskTable_ == nullptr) {
    return {};
  }
  const int row = taskTable_->currentRow();
  if (row < 0 || row >= static_cast<int>(displayedSnapshots_.size())) {
    return {};
  }
  return displayedSnapshots_[static_cast<std::size_t>(row)].taskId;
}

TransferPage::TaskFilter TransferPage::currentFilter() const {
  if (filterDownloadingBtn_ != nullptr && filterDownloadingBtn_->isChecked()) {
    return TaskFilter::kDownloading;
  }
  if (filterSeedingBtn_ != nullptr && filterSeedingBtn_->isChecked()) {
    return TaskFilter::kSeeding;
  }
  if (filterFinishedBtn_ != nullptr && filterFinishedBtn_->isChecked()) {
    return TaskFilter::kFinished;
  }
  if (filterCheckingBtn_ != nullptr && filterCheckingBtn_->isChecked()) {
    return TaskFilter::kChecking;
  }
  if (filterErrorBtn_ != nullptr && filterErrorBtn_->isChecked()) {
    return TaskFilter::kError;
  }
  if (filterRunningBtn_ != nullptr && filterRunningBtn_->isChecked()) {
    return TaskFilter::kRunning;
  }
  if (filterStoppedBtn_ != nullptr && filterStoppedBtn_->isChecked()) {
    return TaskFilter::kStopped;
  }
  if (filterActiveBtn_ != nullptr && filterActiveBtn_->isChecked()) {
    return TaskFilter::kActive;
  }
  if (filterIdleBtn_ != nullptr && filterIdleBtn_->isChecked()) {
    return TaskFilter::kIdle;
  }
  if (filterPausedBtn_ != nullptr && filterPausedBtn_->isChecked()) {
    return TaskFilter::kPaused;
  }
  if (filterUploadPausedBtn_ != nullptr && filterUploadPausedBtn_->isChecked()) {
    return TaskFilter::kUploadPaused;
  }
  if (filterDownloadPausedBtn_ != nullptr && filterDownloadPausedBtn_->isChecked()) {
    return TaskFilter::kDownloadPaused;
  }
  if (filterMovingBtn_ != nullptr && filterMovingBtn_->isChecked()) {
    return TaskFilter::kMoving;
  }
  if (filterErrorDetailBtn_ != nullptr && filterErrorDetailBtn_->isChecked()) {
    return TaskFilter::kErrorDetail;
  }
  return TaskFilter::kAll;
}

void TransferPage::setFilter(TaskFilter f) {
  const int v = static_cast<int>(f);
  if (filterAllBtn_ != nullptr)
    filterAllBtn_->setChecked(v == 0);
  if (filterDownloadingBtn_ != nullptr)
    filterDownloadingBtn_->setChecked(v == 1);
  if (filterSeedingBtn_ != nullptr)
    filterSeedingBtn_->setChecked(v == 2);
  if (filterFinishedBtn_ != nullptr)
    filterFinishedBtn_->setChecked(v == 3);
  if (filterCheckingBtn_ != nullptr)
    filterCheckingBtn_->setChecked(v == 4);
  if (filterErrorBtn_ != nullptr)
    filterErrorBtn_->setChecked(v == 5);
  if (filterRunningBtn_ != nullptr)
    filterRunningBtn_->setChecked(v == 6);
  if (filterStoppedBtn_ != nullptr)
    filterStoppedBtn_->setChecked(v == 7);
  if (filterActiveBtn_ != nullptr)
    filterActiveBtn_->setChecked(v == 8);
  if (filterIdleBtn_ != nullptr)
    filterIdleBtn_->setChecked(v == 9);
  if (filterPausedBtn_ != nullptr)
    filterPausedBtn_->setChecked(v == 10);
  if (filterUploadPausedBtn_ != nullptr)
    filterUploadPausedBtn_->setChecked(v == 11);
  if (filterDownloadPausedBtn_ != nullptr)
    filterDownloadPausedBtn_->setChecked(v == 12);
  if (filterMovingBtn_ != nullptr)
    filterMovingBtn_->setChecked(v == 13);
  if (filterErrorDetailBtn_ != nullptr)
    filterErrorDetailBtn_->setChecked(v == 14);
}

TransferPage::SortKey TransferPage::currentSortKey() const {
  const int idx = sortKeyBox_ != nullptr ? sortKeyBox_->currentIndex() : 2;
  switch (idx) {
    case 0:
      return SortKey::kName;
    case 1:
      return SortKey::kProgress;
    case 3:
      return SortKey::kStatus;
    case 2:
    default:
      return SortKey::kDownloadRate;
  }
}

void TransferPage::setSortKey(SortKey k) {
  if (sortKeyBox_ == nullptr)
    return;
  int idx = 2;
  switch (k) {
    case SortKey::kName:
      idx = 0;
      break;
    case SortKey::kProgress:
      idx = 1;
      break;
    case SortKey::kStatus:
      idx = 3;
      break;
    case SortKey::kDownloadRate:
    default:
      idx = 2;
      break;
  }
  sortKeyBox_->setCurrentIndex(idx);
}

TransferPage::SortOrder TransferPage::currentSortOrder() const {
  const int idx = sortOrderBox_ != nullptr ? sortOrderBox_->currentIndex() : 0;
  return idx == 1 ? SortOrder::kAsc : SortOrder::kDesc;
}

void TransferPage::setSortOrder(SortOrder o) {
  if (sortOrderBox_ == nullptr)
    return;
  sortOrderBox_->setCurrentIndex(o == SortOrder::kAsc ? 1 : 0);
}

QByteArray TransferPage::taskHeaderSaveState() const {
  if (taskTable_ == nullptr || taskTable_->horizontalHeader() == nullptr) {
    return {};
  }
  return taskTable_->horizontalHeader()->saveState();
}

void TransferPage::restoreTaskHeaderState(const QByteArray& state) {
  if (taskTable_ == nullptr || taskTable_->horizontalHeader() == nullptr) {
    return;
  }
  if (!state.isEmpty()) {
    taskTable_->horizontalHeader()->restoreState(state);
  }
}

void TransferPage::updateStats(const std::vector<pfd::core::TaskSnapshot>& snapshots) {
  const int total = static_cast<int>(snapshots.size());
  int downloading = 0;
  int finished = 0;
  int error = 0;
  for (const auto& s : snapshots) {
    if (s.status == pfd::base::TaskStatus::kDownloading ||
        s.status == pfd::base::TaskStatus::kChecking ||
        s.status == pfd::base::TaskStatus::kQueued) {
      ++downloading;
    } else if (s.status == pfd::base::TaskStatus::kFinished) {
      ++finished;
    } else if (s.status == pfd::base::TaskStatus::kError) {
      ++error;
    }
  }
  if (totalStatLabel_ != nullptr)
    totalStatLabel_->setText(QString::number(total));
  if (downloadingStatLabel_ != nullptr)
    downloadingStatLabel_->setText(QString::number(downloading));
  if (finishedStatLabel_ != nullptr)
    finishedStatLabel_->setText(QString::number(finished));
  if (errorStatLabel_ != nullptr)
    errorStatLabel_->setText(QString::number(error));

  QSet<QString> allTags;
  for (const auto& s : snapshots) {
    if (s.tags.isEmpty())
      continue;
    const auto parts = s.tags.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const auto& t : parts) {
      allTags.insert(t.trimmed());
    }
  }

  if (tagFilterLayout_ != nullptr && allTags != lastTagSet_) {
    lastTagSet_ = allTags;

    for (auto* btn : tagFilterButtons_) {
      tagFilterLayout_->removeWidget(btn);
      delete btn;
    }
    tagFilterButtons_.clear();

    auto* allBtn = new QPushButton(QStringLiteral("全部标签"));
    allBtn->setObjectName(QStringLiteral("FilterButton"));
    allBtn->setCheckable(true);
    allBtn->setChecked(activeTagFilter_.isEmpty());
    tagFilterLayout_->addWidget(allBtn);
    tagFilterButtons_.push_back(allBtn);
    connect(allBtn, &QPushButton::clicked, this, [this]() {
      activeTagFilter_.clear();
      for (auto* b : tagFilterButtons_)
        b->setChecked(b == tagFilterButtons_.first());
      emit filterChanged();
    });

    QList<QString> sorted(allTags.begin(), allTags.end());
    std::sort(sorted.begin(), sorted.end());
    for (const auto& tag : sorted) {
      auto* btn = new QPushButton(tag);
      btn->setObjectName(QStringLiteral("FilterButton"));
      btn->setCheckable(true);
      btn->setChecked(activeTagFilter_ == tag);
      tagFilterLayout_->addWidget(btn);
      tagFilterButtons_.push_back(btn);
      connect(btn, &QPushButton::clicked, this, [this, tag]() {
        activeTagFilter_ = tag;
        for (auto* b : tagFilterButtons_) {
          b->setChecked(b->text() == tag);
        }
        emit filterChanged();
      });
    }
  }
}

bool TransferPage::matchFilter(const pfd::core::TaskSnapshot& snapshot, TaskFilter filter) {
  switch (filter) {
    case TaskFilter::kDownloading:
      return snapshot.status == pfd::base::TaskStatus::kDownloading;
    case TaskFilter::kSeeding:
      return snapshot.status == pfd::base::TaskStatus::kSeeding;
    case TaskFilter::kFinished:
      return snapshot.status == pfd::base::TaskStatus::kFinished;
    case TaskFilter::kChecking:
      return snapshot.status == pfd::base::TaskStatus::kChecking;
    case TaskFilter::kError:
    case TaskFilter::kErrorDetail:
      return snapshot.status == pfd::base::TaskStatus::kError;
    case TaskFilter::kRunning:
    case TaskFilter::kActive:
      return snapshot.status == pfd::base::TaskStatus::kDownloading ||
             snapshot.status == pfd::base::TaskStatus::kChecking ||
             snapshot.status == pfd::base::TaskStatus::kQueued ||
             snapshot.status == pfd::base::TaskStatus::kSeeding;
    case TaskFilter::kStopped:
      return snapshot.status == pfd::base::TaskStatus::kPaused ||
             snapshot.status == pfd::base::TaskStatus::kFinished ||
             snapshot.status == pfd::base::TaskStatus::kError;
    case TaskFilter::kIdle:
      return snapshot.status == pfd::base::TaskStatus::kQueued;
    case TaskFilter::kPaused:
      return snapshot.status == pfd::base::TaskStatus::kPaused;
    case TaskFilter::kUploadPaused:
    case TaskFilter::kDownloadPaused:
    case TaskFilter::kMoving:
      return false;
    case TaskFilter::kAll:
    default:
      return true;
  }
}

void TransferPage::updateDetailForSelection() {
  if (detailPanel_ == nullptr)
    return;
  const int row = taskTable_ != nullptr ? taskTable_->currentRow() : -1;
  if (row >= 0 && row < static_cast<int>(displayedSnapshots_.size())) {
    detailPanel_->setSnapshot(displayedSnapshots_[static_cast<std::size_t>(row)]);
    return;
  }
  if (isMemoryMode_ && !memoryTaskId_.isNull()) {
    const int memoryIdx = indexOfTask(memoryTaskId_);
    if (memoryIdx >= 0 && memoryIdx < static_cast<int>(displayedSnapshots_.size())) {
      detailPanel_->setSnapshot(displayedSnapshots_[static_cast<std::size_t>(memoryIdx)]);
      return;
    }
    isMemoryMode_ = false;
    memoryTaskId_ = {};
  }
  detailPanel_->clear();
}

int TransferPage::indexOfTask(const pfd::base::TaskId& id) const {
  if (id.isNull())
    return -1;
  for (int i = 0; i < static_cast<int>(displayedSnapshots_.size()); ++i) {
    if (displayedSnapshots_[static_cast<std::size_t>(i)].taskId == id)
      return i;
  }
  return -1;
}

}  // namespace pfd::ui
