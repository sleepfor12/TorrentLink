#include "ui/rss/rss_items_page.h"

#include <algorithm>

#include <QtCore/QItemSelectionModel>
#include <QtCore/QSet>
#include <QtCore/QSignalBlocker>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtGui/QKeySequence>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>

#include "base/format.h"
#include "base/types.h"
#include "core/external_player.h"
#include "core/rss/rss_dedup.h"
#include "core/rss/rss_service.h"
#include "core/rss/rss_types.h"
#include "core/task_snapshot.h"
#include "ui/input_ime_utils.h"

namespace pfd::ui::rss {

namespace {

QString linkKindLabel(const pfd::core::rss::RssItem& it) {
  const bool hasMagnet = !it.magnet.isEmpty();
  const bool hasTorrent = !it.torrent_url.isEmpty();
  if (hasMagnet && hasTorrent) return QStringLiteral("磁力+种子");
  if (hasMagnet) return QStringLiteral("磁力");
  if (hasTorrent) return QStringLiteral("种子");
  return QStringLiteral("无");
}

const pfd::core::TaskSnapshot* findTaskSnapshotForItem(
    const pfd::core::rss::RssItem& it,
    const std::vector<pfd::core::TaskSnapshot>& snaps) {
  if (!it.magnet.isEmpty()) {
    const QString ih = pfd::core::rss::RssDedup::extractInfoHash(it.magnet);
    if (!ih.isEmpty()) {
      for (const auto& t : snaps) {
        if (!t.infoHashV1.isEmpty() && t.infoHashV1.compare(ih, Qt::CaseInsensitive) == 0) {
          return &t;
        }
      }
    }
  }
  const QString title = it.title.trimmed();
  if (it.downloaded && title.size() >= 8) {
    const QString frag = title.size() > 40 ? title.left(40) : title;
    for (const auto& s : snaps) {
      if (s.name.contains(frag)) {
        return &s;
      }
    }
  }
  return nullptr;
}

QString resolveSavePathFromTasks(
    const pfd::core::rss::RssItem& it,
    const std::function<std::vector<pfd::core::TaskSnapshot>()>& taskSnapshots) {
  if (!static_cast<bool>(taskSnapshots)) {
    return {};
  }
  const std::vector<pfd::core::TaskSnapshot> snaps = taskSnapshots();
  const pfd::core::TaskSnapshot* p = findTaskSnapshotForItem(it, snaps);
  return p ? p->savePath : QString{};
}

QString taskStatusLabelZh(pfd::base::TaskStatus s) {
  using pfd::base::TaskStatus;
  switch (s) {
    case TaskStatus::kUnknown:
      return QStringLiteral("未知");
    case TaskStatus::kQueued:
      return QStringLiteral("排队");
    case TaskStatus::kChecking:
      return QStringLiteral("校验");
    case TaskStatus::kDownloading:
      return QStringLiteral("下载");
    case TaskStatus::kSeeding:
      return QStringLiteral("做种");
    case TaskStatus::kPaused:
      return QStringLiteral("暂停");
    case TaskStatus::kFinished:
      return QStringLiteral("完成");
    case TaskStatus::kError:
      return QStringLiteral("错误");
  }
  return QStringLiteral("未知");
}

QString taskProgressCellText(const pfd::core::TaskSnapshot& s) {
  using pfd::base::TaskStatus;
  QString st = taskStatusLabelZh(s.status);
  double p = s.progress01();
  if (pfd::base::isFinished(s.status)) {
    p = 1.0;
  }
  if (s.totalBytes > 0 || pfd::base::isFinished(s.status) || s.status == TaskStatus::kChecking ||
      s.status == TaskStatus::kDownloading || s.status == TaskStatus::kPaused) {
    st += QStringLiteral(" ") + pfd::base::formatPercent(p);
  }
  if (s.status == TaskStatus::kDownloading && s.downloadRate > 0) {
    st += QStringLiteral(" · ") + pfd::base::formatRate(s.downloadRate);
  }
  if (s.status == TaskStatus::kError && !s.errorMessage.trimmed().isEmpty()) {
    QString err = s.errorMessage.trimmed();
    if (err.size() > 28) {
      err = err.left(28) + QStringLiteral("…");
    }
    st += QStringLiteral(" ") + err;
  }
  return st;
}

QString formatRssTaskDetailHtml(const pfd::core::TaskSnapshot& s) {
  using pfd::base::TaskStatus;
  QString h;
  h += QStringLiteral("<p><b>状态</b>：%1</p>").arg(taskStatusLabelZh(s.status).toHtmlEscaped());
  double p = s.progress01();
  if (pfd::base::isFinished(s.status)) {
    p = 1.0;
  }
  h += QStringLiteral("<p><b>进度</b>：%1</p>").arg(pfd::base::formatPercent(p));
  if (s.totalBytes > 0) {
    h += QStringLiteral("<p><b>已下/总计</b>：%1 / %2</p>")
             .arg(pfd::base::formatBytes(s.downloadedBytes), pfd::base::formatBytes(s.totalBytes));
  }
  if (s.status == TaskStatus::kDownloading && s.downloadRate > 0) {
    h += QStringLiteral("<p><b>下载速率</b>：%1</p>").arg(pfd::base::formatRate(s.downloadRate));
  }
  if (!s.savePath.isEmpty()) {
    h += QStringLiteral("<p><b>保存路径</b>：<code style=\"word-break:break-all;\">%1</code></p>")
             .arg(s.savePath.toHtmlEscaped());
  }
  if (s.status == TaskStatus::kError && !s.errorMessage.isEmpty()) {
    h += QStringLiteral("<p><b>错误</b>：%1</p>").arg(s.errorMessage.toHtmlEscaped());
  }
  return h;
}

}  // namespace

RssItemsPage::RssItemsPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
  bindSignals();
}

void RssItemsPage::setService(pfd::core::rss::RssService* service) {
  service_ = service;
  refreshTable();
}

void RssItemsPage::setUiHelpers(std::function<void(const QString&)> appendItemsLog,
                              std::function<QString()> defaultSaveRoot,
                              std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots) {
  appendItemsLog_ = std::move(appendItemsLog);
  defaultSaveRoot_ = std::move(defaultSaveRoot);
  taskSnapshots_ = std::move(taskSnapshots);
  updateActionStates();
}

void RssItemsPage::userLog(const QString& msg) const {
  if (appendItemsLog_) {
    appendItemsLog_(msg);
  }
}

QStringList RssItemsPage::selectedItemIdsOrdered() const {
  QStringList out;
  if (!itemTable_) {
    return out;
  }
  const auto rows = itemTable_->selectionModel()->selectedRows();
  QList<int> rowIndices;
  rowIndices.reserve(rows.size());
  for (const QModelIndex& ix : rows) {
    rowIndices.append(ix.row());
  }
  std::sort(rowIndices.begin(), rowIndices.end());
  for (int r : rowIndices) {
    if (QTableWidgetItem* c = itemTable_->item(r, 0)) {
      out.append(c->data(Qt::UserRole).toString());
    }
  }
  return out;
}

const pfd::core::rss::RssItem* RssItemsPage::findItem(const QString& id) const {
  if (!service_ || id.isEmpty()) {
    return nullptr;
  }
  for (const auto& it : service_->items()) {
    if (it.id == id) {
      return &it;
    }
  }
  return nullptr;
}

bool RssItemsPage::selectionHasDownloadable() const {
  for (const QString& id : selectedItemIdsOrdered()) {
    const auto* it = findItem(id);
    if (it && (!it->magnet.isEmpty() || !it->torrent_url.isEmpty())) {
      return true;
    }
  }
  return false;
}

void RssItemsPage::updateActionStates() {
  const QStringList ids = selectedItemIdsOrdered();
  const bool hasSel = !ids.isEmpty();
  const bool multi = ids.size() > 1;
  const bool canDl = hasSel && selectionHasDownloadable();

  if (downloadBtn_) {
    downloadBtn_->setEnabled(canDl);
  }
  if (copyMagnetBtn_) {
    copyMagnetBtn_->setEnabled(hasSel);
  }
  if (openFolderBtn_) {
    openFolderBtn_->setEnabled(hasSel && !multi);
  }
  if (playBtn_) {
    playBtn_->setEnabled(hasSel && !multi);
  }
  if (ignoreBtn_) {
    ignoreBtn_->setEnabled(hasSel);
  }
}

void RssItemsPage::refreshTable() {
  QSet<QString> keepSelectedIds;
  const int prevRow = itemTable_->currentRow();
  if (itemTable_->selectionModel()) {
    for (const QModelIndex& ix : itemTable_->selectionModel()->selectedRows()) {
      if (QTableWidgetItem* c = itemTable_->item(ix.row(), 0)) {
        keepSelectedIds.insert(c->data(Qt::UserRole).toString());
      }
    }
  }
  if (keepSelectedIds.isEmpty() && prevRow >= 0) {
    if (QTableWidgetItem* titleCell = itemTable_->item(prevRow, 0)) {
      keepSelectedIds.insert(titleCell->data(Qt::UserRole).toString());
    }
  }

  itemTable_->setRowCount(0);

  if (!service_) {
    detailView_->clear();
    updateActionStates();
    return;
  }

  const QString filter = searchEdit_->text().trimmed().toLower();
  const auto& allItems = service_->items();

  std::vector<const pfd::core::rss::RssItem*> filtered;
  filtered.reserve(allItems.size());
  for (const auto& it : allItems) {
    if (it.ignored) continue;
    if (!filter.isEmpty() && !it.title.toLower().contains(filter)) continue;
    filtered.push_back(&it);
  }

  std::sort(filtered.begin(), filtered.end(),
            [](const pfd::core::rss::RssItem* a, const pfd::core::rss::RssItem* b) {
              return a->published_at > b->published_at;
            });

  itemTable_->setRowCount(static_cast<int>(filtered.size()));
  for (int i = 0; i < static_cast<int>(filtered.size()); ++i) {
    const auto* it = filtered[static_cast<std::size_t>(i)];

    QString feedName;
    auto feed = service_->findFeed(it->feed_id);
    if (feed.has_value()) feedName = feed->title;

    auto* titleItem = new QTableWidgetItem(it->title);
    titleItem->setData(Qt::UserRole, it->id);
    itemTable_->setItem(i, 0, titleItem);
    itemTable_->setItem(i, 1, new QTableWidgetItem(feedName));
    itemTable_->setItem(i, 2, new QTableWidgetItem(
        it->published_at.isValid()
            ? it->published_at.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
            : QString()));
    itemTable_->setItem(i, 3, new QTableWidgetItem(linkKindLabel(*it)));

    QString ruleHit;
    {
      const auto matches = service_->evaluateItem(*it);
      int hitCount = 0;
      for (const auto& m : matches) {
        if (m.matched) ++hitCount;
      }
      ruleHit = hitCount > 0 ? QStringLiteral("命中 %1 条").arg(hitCount) : QStringLiteral("-");
    }
    itemTable_->setItem(i, 4, new QTableWidgetItem(ruleHit));

    QString seriesHit;
    auto ep = service_->parseEpisode(it->title);
    if (ep.has_value()) {
      auto matchedSub = service_->matchSeries(*it);
      if (matchedSub.has_value()) {
        seriesHit = QStringLiteral("%1 E%2").arg(matchedSub->name).arg(ep->episode);
      } else {
        seriesHit = QStringLiteral("E%1").arg(ep->episode);
      }
    }
    itemTable_->setItem(i, 5, new QTableWidgetItem(seriesHit.isEmpty() ? QStringLiteral("-") : seriesHit));
    itemTable_->setItem(i, 6, new QTableWidgetItem(
        it->downloaded ? QStringLiteral("已下载") : QStringLiteral("-")));
  }

  refreshTaskProgressCells();

  if (keepSelectedIds.isEmpty()) {
    detailView_->clear();
    updateActionStates();
    return;
  }

  int anchorRow = -1;
  {
    QSignalBlocker blocker(itemTable_);
    itemTable_->clearSelection();
    for (int i = 0; i < itemTable_->rowCount(); ++i) {
      if (QTableWidgetItem* cell = itemTable_->item(i, 0)) {
        const QString id = cell->data(Qt::UserRole).toString();
        if (keepSelectedIds.contains(id)) {
          itemTable_->selectRow(i);
          if (anchorRow < 0) {
            anchorRow = i;
          }
        }
      }
    }
    if (anchorRow >= 0) {
      itemTable_->setCurrentCell(anchorRow, 0);
    }
  }

  if (anchorRow >= 0) {
    onSelectionChanged();
  } else {
    detailView_->clear();
  }
  updateActionStates();
}

void RssItemsPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(10);

  auto* top = new QHBoxLayout();
  top->setSpacing(8);
  searchEdit_ = new QLineEdit(this);
  searchEdit_->setPlaceholderText(QStringLiteral("按标题搜索"));
  refreshBtn_ = new QPushButton(QStringLiteral("刷新列表"), this);
  downloadBtn_ = new QPushButton(QStringLiteral("下载选中"), this);
  downloadBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  copyMagnetBtn_ = new QPushButton(QStringLiteral("复制链接"), this);
  openFolderBtn_ = new QPushButton(QStringLiteral("打开文件夹"), this);
  playBtn_ = new QPushButton(QStringLiteral("播放"), this);
  ignoreBtn_ = new QPushButton(QStringLiteral("忽略"), this);
  top->addWidget(searchEdit_, 1);
  top->addWidget(refreshBtn_);
  top->addWidget(downloadBtn_);
  top->addWidget(copyMagnetBtn_);
  top->addWidget(openFolderBtn_);
  top->addWidget(playBtn_);
  top->addWidget(ignoreBtn_);
  root->addLayout(top);

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  itemTable_ = new QTableWidget(splitter);
  itemTable_->setColumnCount(8);
  itemTable_->setHorizontalHeaderLabels(
      {QStringLiteral("标题"), QStringLiteral("来源"), QStringLiteral("发布时间"),
       QStringLiteral("资源"), QStringLiteral("规则命中"),
       QStringLiteral("剧集"), QStringLiteral("下载状态"), QStringLiteral("任务进度")});
  itemTable_->horizontalHeader()->setStretchLastSection(true);
  itemTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  itemTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  itemTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  itemTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  itemTable_->setContextMenuPolicy(Qt::CustomContextMenu);
  itemTable_->setRowCount(0);
  itemTable_->installEventFilter(this);

  detailView_ = new QTextBrowser(splitter);
  detailView_->setPlaceholderText(QStringLiteral("选择条目后查看详情。"));
  detailView_->setOpenExternalLinks(true);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);

  root->addWidget(splitter, 1);
  updateActionStates();
}

void RssItemsPage::bindSignals() {
  connect(downloadBtn_, &QPushButton::clicked, this, &RssItemsPage::onDownloadClicked);
  connect(refreshBtn_, &QPushButton::clicked, this, &RssItemsPage::refreshTable);
  connectImeAwareLineEditApply(this, searchEdit_, 120, [this]() { onFilterChanged(); });
  connect(copyMagnetBtn_, &QPushButton::clicked, this, &RssItemsPage::onCopyMagnet);
  connect(openFolderBtn_, &QPushButton::clicked, this, &RssItemsPage::onOpenFolder);
  connect(playBtn_, &QPushButton::clicked, this, &RssItemsPage::onPlayClicked);
  connect(ignoreBtn_, &QPushButton::clicked, this, &RssItemsPage::onIgnoreItem);
  connect(itemTable_, &QTableWidget::currentCellChanged, this,
          [this](int, int, int, int) { onSelectionChanged(); });
  if (itemTable_->selectionModel()) {
    connect(itemTable_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() {
              updateActionStates();
              onSelectionChanged();
            });
  }
  connect(itemTable_, &QTableWidget::customContextMenuRequested, this,
          &RssItemsPage::showRowContextMenu);
}

void RssItemsPage::onSelectionChanged() {
  if (!service_) return;
  const auto selectedRows = itemTable_->selectionModel()->selectedRows();
  if (selectedRows.size() > 1) {
    detailView_->setHtml(QStringLiteral("<p>已选择 <b>%1</b> 条。可使用顶部按钮或右键菜单批量下载、忽略；复制链接按列表顺序取第一条可复制的条目。</p>")
                             .arg(selectedRows.size()));
    return;
  }

  const int row = itemTable_->currentRow();
  if (row < 0) {
    detailView_->clear();
    return;
  }

  const QString itemId = itemTable_->item(row, 0)->data(Qt::UserRole).toString();
  for (const auto& it : service_->items()) {
    if (it.id != itemId) continue;

    service_->markItemRead(itemId);

    QString html;
    html += QStringLiteral("<h3>%1</h3>").arg(it.title.toHtmlEscaped());
    if (!it.link.isEmpty()) {
      html += QStringLiteral("<p><a href=\"%1\">%1</a></p>").arg(it.link.toHtmlEscaped());
    }
    if (it.published_at.isValid()) {
      html += QStringLiteral("<p><b>发布时间：</b>%1</p>")
                  .arg(it.published_at.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    }
    if (!it.magnet.isEmpty()) {
      html += QStringLiteral("<p><b>磁力链接：</b><br/><code style=\"word-break:break-all;\">%1</code></p>")
                  .arg(it.magnet.toHtmlEscaped());
    }
    if (!it.torrent_url.isEmpty()) {
      html += QStringLiteral("<p><b>Torrent：</b><a href=\"%1\">%1</a></p>")
                  .arg(it.torrent_url.toHtmlEscaped());
    }

    auto ep = service_->parseEpisode(it.title);
    if (ep.has_value()) {
      html += QStringLiteral("<p><b>剧集识别：</b>%1 S%2E%3 %4</p>")
                  .arg(ep->series_name.toHtmlEscaped())
                  .arg(ep->season >= 0 ? ep->season : 0, 2, 10, QLatin1Char('0'))
                  .arg(ep->episode, 2, 10, QLatin1Char('0'))
                  .arg(ep->quality.toHtmlEscaped());
    }

    if (!it.summary.isEmpty()) {
      html += QStringLiteral("<hr/><p>%1</p>").arg(it.summary.toHtmlEscaped());
    }

    const auto matches = service_->evaluateItem(it);
    if (!matches.empty()) {
      html += QStringLiteral("<hr/><p><b>规则评估：</b></p>"
                             "<table style=\"border-collapse:collapse;width:100%;\">"
                             "<tr style=\"background:#f5f8ff;\"><th style=\"text-align:left;padding:4px 8px;\">规则</th>"
                             "<th style=\"text-align:left;padding:4px 8px;\">结果</th>"
                             "<th style=\"text-align:left;padding:4px 8px;\">原因</th></tr>");
      for (const auto& m : matches) {
        const QString icon = m.matched ? QStringLiteral("✓") : QStringLiteral("✗");
        const QString color = m.matched ? QStringLiteral("#22c55e") : QStringLiteral("#9ca3af");
        html += QStringLiteral("<tr><td style=\"padding:4px 8px;\">%1</td>"
                               "<td style=\"padding:4px 8px;color:%2;font-weight:700;\">%3</td>"
                               "<td style=\"padding:4px 8px;\">%4</td></tr>")
                    .arg(m.rule_name.toHtmlEscaped(), color, icon, m.reason.toHtmlEscaped());
      }
      html += QStringLiteral("</table>");
    }

    html += QStringLiteral("<hr/><p><b>关联任务</b></p>");
    if (static_cast<bool>(taskSnapshots_)) {
      const std::vector<pfd::core::TaskSnapshot> snaps = taskSnapshots_();
      const pfd::core::TaskSnapshot* snap = findTaskSnapshotForItem(it, snaps);
      if (snap) {
        html += formatRssTaskDetailHtml(*snap);
      } else {
        html += QStringLiteral(
            "<p>当前传输列表中未匹配到对应任务。通常需要条目含磁力且能解析出 info_hash；纯 Torrent "
            "直链在任务尚未带 hash 前可能暂时无法关联，可在「传输」页查看进度。</p>");
      }
    } else {
      html += QStringLiteral("<p>（无任务快照）</p>");
    }

    detailView_->setHtml(html);
    return;
  }
}

void RssItemsPage::onDownloadClicked() {
  if (!service_) return;
  const QStringList ids = selectedItemIdsOrdered();
  if (ids.isEmpty()) return;

  int dispatched = 0;
  for (const QString& id : ids) {
    if (service_->downloadItem(id)) {
      ++dispatched;
    }
  }

  if (dispatched > 0) {
    service_->saveState();
  }
  if (dispatched == 0) {
    QMessageBox::warning(this, QStringLiteral("下载"),
                         QStringLiteral("所选条目没有可用的磁力或 Torrent 链接。"));
  } else {
    userLog(QStringLiteral("[RSS] 已请求下载 %1 个条目").arg(dispatched));
  }
  refreshTable();
}

void RssItemsPage::onCopyMagnet() {
  if (!service_) return;
  const QStringList ids = selectedItemIdsOrdered();
  if (ids.isEmpty()) return;

  for (const QString& id : ids) {
    const auto* it = findItem(id);
    if (!it) continue;
    const QString toCopy =
        !it->magnet.isEmpty() ? it->magnet : it->torrent_url;
    if (toCopy.isEmpty()) {
      continue;
    }
    QApplication::clipboard()->setText(toCopy);
    userLog(QStringLiteral("[RSS] 已复制链接：%1").arg(it->title.left(80)));
    return;
  }
  QMessageBox::information(this, QStringLiteral("复制链接"),
                             QStringLiteral("所选条目均没有磁力或 Torrent 链接。"));
}

QString RssItemsPage::resolveSelectedItemPath(const QString& actionName) const {
  const QStringList ids = selectedItemIdsOrdered();
  if (ids.size() != 1) {
    if (ids.size() > 1) {
      QMessageBox::information(const_cast<RssItemsPage*>(this), actionName,
                               QStringLiteral("请只选择一条条目。"));
    }
    return {};
  }

  const auto* it = findItem(ids.front());
  if (!it) return {};

  if (!it->downloaded) {
    QMessageBox::information(const_cast<RssItemsPage*>(this), actionName,
                             QStringLiteral("该条目尚未标记为已下载。"));
    return {};
  }

  QString path = it->download_save_path;
  if (path.isEmpty()) {
    path = resolveSavePathFromTasks(*it, taskSnapshots_);
  }
  if (path.isEmpty() && defaultSaveRoot_) {
    path = defaultSaveRoot_();
    if (!path.isEmpty()) {
      userLog(QStringLiteral("[RSS] 使用默认下载目录（RSS 未记录具体路径）。"));
    }
  }

  if (path.isEmpty()) {
    QMessageBox::information(
        const_cast<RssItemsPage*>(this), actionName,
        QStringLiteral("RSS 未记录保存路径，且未能从当前任务列表匹配到该条目的磁力任务。\n"
                       "请在主界面的下载列表中定位任务后打开所在文件夹。"));
    return {};
  }
  return path;
}

void RssItemsPage::onOpenFolder() {
  if (!service_) return;
  const QString path = resolveSelectedItemPath(QStringLiteral("打开文件夹"));
  if (path.isEmpty()) return;
  pfd::core::ExternalPlayer::openFolder(path);
}

void RssItemsPage::onPlayClicked() {
  if (!service_) return;
  const QString path = resolveSelectedItemPath(QStringLiteral("播放"));
  if (path.isEmpty()) return;

  const QString cmd = service_->settings().external_player_command.trimmed();
  if (cmd.isEmpty()) {
    const auto result = pfd::core::ExternalPlayer::openWithDefault(path);
    if (result == pfd::core::ExternalPlayer::Result::FileNotFound) {
      userLog(QStringLiteral("[RSS] 播放失败：文件不存在。"));
    } else if (result == pfd::core::ExternalPlayer::Result::LaunchFailed) {
      userLog(QStringLiteral("[RSS] 播放失败：无法启动系统默认播放器。"));
    }
    return;
  }

  const auto cmdResult = pfd::core::ExternalPlayer::openWithCommand(cmd, path);
  if (cmdResult == pfd::core::ExternalPlayer::Result::FileNotFound) {
    userLog(QStringLiteral("[RSS] 播放失败：文件不存在。"));
    return;
  }
  if (cmdResult == pfd::core::ExternalPlayer::Result::LaunchFailed) {
    userLog(QStringLiteral("[RSS] 自定义播放器启动失败，尝试使用系统默认播放器。"));
    const auto fallback = pfd::core::ExternalPlayer::openWithDefault(path);
    if (fallback == pfd::core::ExternalPlayer::Result::LaunchFailed) {
      userLog(QStringLiteral("[RSS] 回退失败：系统默认播放器也无法启动。"));
    }
  }
}

void RssItemsPage::onIgnoreItem() {
  if (!service_) return;
  const QStringList ids = selectedItemIdsOrdered();
  if (ids.isEmpty()) return;

  service_->markItemsIgnored(ids);
  service_->saveState();
  userLog(QStringLiteral("[RSS] 已忽略 %1 条条目").arg(ids.size()));
  refreshTable();
}

void RssItemsPage::showRowContextMenu(const QPoint& pos) {
  QMenu menu(this);
  auto* aDl = menu.addAction(QStringLiteral("下载选中"));
  aDl->setEnabled(downloadBtn_->isEnabled());
  connect(aDl, &QAction::triggered, this, &RssItemsPage::onDownloadClicked);

  auto* aCopy = menu.addAction(QStringLiteral("复制链接"));
  aCopy->setEnabled(copyMagnetBtn_->isEnabled());
  connect(aCopy, &QAction::triggered, this, &RssItemsPage::onCopyMagnet);

  auto* aOpen = menu.addAction(QStringLiteral("打开文件夹"));
  aOpen->setEnabled(openFolderBtn_->isEnabled());
  connect(aOpen, &QAction::triggered, this, &RssItemsPage::onOpenFolder);

  auto* aPlay = menu.addAction(QStringLiteral("播放"));
  aPlay->setEnabled(playBtn_->isEnabled());
  connect(aPlay, &QAction::triggered, this, &RssItemsPage::onPlayClicked);

  menu.addSeparator();
  auto* aIgn = menu.addAction(QStringLiteral("忽略"));
  aIgn->setEnabled(ignoreBtn_->isEnabled());
  connect(aIgn, &QAction::triggered, this, &RssItemsPage::onIgnoreItem);

  menu.exec(itemTable_->viewport()->mapToGlobal(pos));
}

void RssItemsPage::onFilterChanged() {
  refreshTable();
}

void RssItemsPage::keyPressEvent(QKeyEvent* event) {
  if (event->matches(QKeySequence::Copy)) {
    onCopyMagnet();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

bool RssItemsPage::eventFilter(QObject* watched, QEvent* event) {
  if (watched == itemTable_ && event->type() == QEvent::KeyPress) {
    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->matches(QKeySequence::Copy)) {
      onCopyMagnet();
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

void RssItemsPage::refreshTaskProgressCells() {
  if (!itemTable_ || itemTable_->columnCount() < 8) {
    return;
  }
  if (!static_cast<bool>(taskSnapshots_)) {
    for (int i = 0; i < itemTable_->rowCount(); ++i) {
      QTableWidgetItem* progCell = itemTable_->item(i, 7);
      if (!progCell) {
        progCell = new QTableWidgetItem(QStringLiteral("—"));
        itemTable_->setItem(i, 7, progCell);
      } else {
        progCell->setText(QStringLiteral("—"));
      }
    }
    return;
  }
  const std::vector<pfd::core::TaskSnapshot> snaps = taskSnapshots_();
  const int rows = itemTable_->rowCount();
  for (int i = 0; i < rows; ++i) {
    QTableWidgetItem* idCell = itemTable_->item(i, 0);
    if (!idCell) {
      continue;
    }
    const QString id = idCell->data(Qt::UserRole).toString();
    const pfd::core::rss::RssItem* rowItem = findItem(id);
    QString text = QStringLiteral("—");
    if (rowItem) {
      if (const pfd::core::TaskSnapshot* snap = findTaskSnapshotForItem(*rowItem, snaps)) {
        text = taskProgressCellText(*snap);
      }
    }
    QTableWidgetItem* progCell = itemTable_->item(i, 7);
    if (!progCell) {
      progCell = new QTableWidgetItem(text);
      itemTable_->setItem(i, 7, progCell);
    } else {
      progCell->setText(text);
    }
  }
}

}  // namespace pfd::ui::rss
