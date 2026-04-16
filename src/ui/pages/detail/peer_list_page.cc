#include "ui/pages/detail/peer_list_page.h"

#include <QtCore/QTimer>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "base/format.h"

namespace pfd::ui {

PeerListPage::PeerListPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void PeerListPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);

  table_ = new QTableWidget(this);
  table_->setColumnCount(9);
  table_->setHorizontalHeaderLabels(
      {QStringLiteral("IP"), QStringLiteral("端口"), QStringLiteral("客户端"),
       QStringLiteral("进度"), QStringLiteral("下载速度"), QStringLiteral("上传速度"),
       QStringLiteral("已下载"), QStringLiteral("已上传"), QStringLiteral("标志")});
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->setVisible(false);
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->setAlternatingRowColors(true);
  root->addWidget(table_);

  refreshTimer_ = new QTimer(this);
  refreshTimer_->setInterval(3000);
  connect(refreshTimer_, &QTimer::timeout, this, &PeerListPage::reload);
}

void PeerListPage::setQueryFn(QueryPeersFn fn) {
  queryFn_ = std::move(fn);
}

void PeerListPage::setSnapshot(const pfd::core::TaskSnapshot& snap) {
  taskId_ = snap.taskId;
  if (isVisible()) {
    reload();
  }
}

void PeerListPage::clear() {
  taskId_ = {};
  table_->setRowCount(0);
}

void PeerListPage::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  reload();
  refreshTimer_->start();
}

void PeerListPage::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
  refreshTimer_->stop();
}

void PeerListPage::reload() {
  if (!queryFn_ || taskId_.isNull())
    return;
  const auto peers = queryFn_(taskId_);
  table_->setRowCount(static_cast<int>(peers.size()));
  for (int i = 0; i < static_cast<int>(peers.size()); ++i) {
    const auto& p = peers[static_cast<size_t>(i)];
    auto makeItem = [](const QString& text) {
      auto* item = new QTableWidgetItem(text);
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      return item;
    };
    table_->setItem(i, 0, makeItem(p.ip));
    table_->setItem(i, 1, makeItem(QString::number(p.port)));
    table_->setItem(i, 2, makeItem(p.client));
    table_->setItem(i, 3, makeItem(pfd::base::formatPercent(p.progress)));
    table_->setItem(i, 4, makeItem(pfd::base::formatRate(p.downloadRate)));
    table_->setItem(i, 5, makeItem(pfd::base::formatRate(p.uploadRate)));
    table_->setItem(i, 6, makeItem(pfd::base::formatBytes(p.totalDownloaded)));
    table_->setItem(i, 7, makeItem(pfd::base::formatBytes(p.totalUploaded)));
    table_->setItem(i, 8, makeItem(p.flags));
  }
}

}  // namespace pfd::ui
