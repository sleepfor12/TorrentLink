#include "ui/rss/rss_feeds_page.h"

#include <QtCore/QUrl>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "core/rss/rss_opml.h"
#include "core/rss/rss_service.h"

namespace pfd::ui::rss {

RssFeedsPage::RssFeedsPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
  bindSignals();
}

void RssFeedsPage::setService(pfd::core::rss::RssService* service) {
  service_ = service;
  refreshTable();
}

void RssFeedsPage::refreshTable() {
  feedTable_->setRowCount(0);
  if (!service_)
    return;

  const auto& feeds = service_->feeds();
  feedTable_->setRowCount(static_cast<int>(feeds.size()));
  for (int i = 0; i < static_cast<int>(feeds.size()); ++i) {
    const auto& f = feeds[static_cast<std::size_t>(i)];
    feedTable_->setItem(i, 0, new QTableWidgetItem(f.title.isEmpty() ? f.url : f.title));
    feedTable_->setItem(i, 1, new QTableWidgetItem(f.url));
    feedTable_->setItem(
        i, 2, new QTableWidgetItem(f.enabled ? QStringLiteral("启用") : QStringLiteral("暂停")));
    feedTable_->setItem(
        i, 3,
        new QTableWidgetItem(f.last_refreshed_at.isValid()
                                 ? f.last_refreshed_at.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                                 : QStringLiteral("从未")));
    feedTable_->setItem(i, 4,
                        new QTableWidgetItem(f.auto_download_enabled ? QStringLiteral("已开启")
                                                                     : QStringLiteral("关闭")));
    feedTable_->setItem(i, 5, new QTableWidgetItem(f.last_error));
    feedTable_->item(i, 0)->setData(Qt::UserRole, f.id);
  }
}

void RssFeedsPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(10);

  auto* row = new QHBoxLayout();
  row->setSpacing(8);
  feedUrlEdit_ = new QLineEdit(this);
  feedUrlEdit_->setPlaceholderText(QStringLiteral("输入订阅地址（https://example.com/rss.xml）"));
  addFeedBtn_ = new QPushButton(QStringLiteral("添加订阅"), this);
  addFeedBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  refreshAllBtn_ = new QPushButton(QStringLiteral("全部刷新"), this);
  removeBtn_ = new QPushButton(QStringLiteral("删除选中"), this);
  importBtn_ = new QPushButton(QStringLiteral("导入 OPML"), this);
  exportBtn_ = new QPushButton(QStringLiteral("导出 OPML"), this);
  row->addWidget(feedUrlEdit_, 1);
  row->addWidget(addFeedBtn_);
  row->addWidget(refreshAllBtn_);
  row->addWidget(removeBtn_);
  row->addWidget(importBtn_);
  row->addWidget(exportBtn_);
  root->addLayout(row);

  feedTable_ = new QTableWidget(this);
  feedTable_->setColumnCount(6);
  feedTable_->setHorizontalHeaderLabels({QStringLiteral("名称"), QStringLiteral("URL"),
                                         QStringLiteral("状态"), QStringLiteral("上次刷新"),
                                         QStringLiteral("自动下载"), QStringLiteral("错误")});
  feedTable_->horizontalHeader()->setStretchLastSection(true);
  feedTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  feedTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  feedTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  feedTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  feedTable_->setContextMenuPolicy(Qt::CustomContextMenu);
  feedTable_->setRowCount(0);
  root->addWidget(feedTable_, 1);

  auto* hint =
      new QLabel(QStringLiteral("提示：自动下载默认关闭，需在设置页、订阅源和规则页三处均手动开启后"
                                "才会生效。表格支持多选，右键可刷新、编辑、复制链接等。"),
                 this);
  hint->setStyleSheet(QStringLiteral("color:#6b7280;font-size:12px;"));
  root->addWidget(hint);
}

void RssFeedsPage::bindSignals() {
  connect(addFeedBtn_, &QPushButton::clicked, this, &RssFeedsPage::onAddFeed);
  connect(refreshAllBtn_, &QPushButton::clicked, this, &RssFeedsPage::onRefreshAll);
  connect(removeBtn_, &QPushButton::clicked, this, &RssFeedsPage::onRemoveSelected);
  connect(importBtn_, &QPushButton::clicked, this, &RssFeedsPage::onImportOpml);
  connect(exportBtn_, &QPushButton::clicked, this, &RssFeedsPage::onExportOpml);
  connect(feedTable_, &QTableWidget::customContextMenuRequested, this,
          &RssFeedsPage::showFeedContextMenu);
}

QStringList RssFeedsPage::selectedFeedIds() const {
  QStringList out;
  if (feedTable_ == nullptr || feedTable_->selectionModel() == nullptr) {
    return out;
  }
  const QModelIndexList rows = feedTable_->selectionModel()->selectedRows();
  out.reserve(rows.size());
  for (const QModelIndex& idx : rows) {
    QTableWidgetItem* it = feedTable_->item(idx.row(), 0);
    if (it == nullptr)
      continue;
    const QString id = it->data(Qt::UserRole).toString();
    if (!id.isEmpty()) {
      out.push_back(id);
    }
  }
  return out;
}

void RssFeedsPage::removeFeedsByIds(const QStringList& ids) {
  if (!service_ || ids.isEmpty())
    return;
  for (const QString& id : ids) {
    service_->removeFeed(id);
  }
  service_->saveState();
  refreshTable();
}

void RssFeedsPage::editFeedById(const QString& feedId) {
  if (!service_)
    return;
  const auto f = service_->findFeed(feedId);
  if (!f.has_value())
    return;

  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("编辑订阅"));
  dlg.resize(480, 120);
  auto* form = new QFormLayout(&dlg);
  auto* titleEdit = new QLineEdit(f->title, &dlg);
  auto* urlEdit = new QLineEdit(f->url, &dlg);
  form->addRow(QStringLiteral("显示名称"), titleEdit);
  form->addRow(QStringLiteral("订阅 URL"), urlEdit);
  auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  form->addRow(box);
  QObject::connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  QObject::connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

  if (dlg.exec() != QDialog::Accepted)
    return;

  const QString newUrl = urlEdit->text().trimmed();
  if (newUrl.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("编辑订阅"), QStringLiteral("订阅 URL 不能为空。"));
    return;
  }

  pfd::core::rss::RssFeed u = *f;
  u.title = titleEdit->text().trimmed();
  if (u.title.isEmpty()) {
    u.title = newUrl;
  }
  u.url = newUrl;
  service_->upsertFeed(u);
  service_->saveState();
  refreshTable();
}

void RssFeedsPage::showFeedContextMenu(const QPoint& pos) {
  if (!service_ || feedTable_ == nullptr)
    return;

  QModelIndexList rows = feedTable_->selectionModel()->selectedRows();
  if (rows.isEmpty()) {
    const int r = feedTable_->rowAt(pos.y());
    if (r >= 0) {
      feedTable_->selectRow(r);
      rows = feedTable_->selectionModel()->selectedRows();
    }
  }
  if (rows.isEmpty())
    return;

  const QPoint globalPos = feedTable_->viewport()->mapToGlobal(pos);
  QMenu menu(this);

  if (rows.size() == 1) {
    const int row = rows.front().row();
    QTableWidgetItem* nameItem = feedTable_->item(row, 0);
    if (nameItem == nullptr)
      return;
    const QString feedId = nameItem->data(Qt::UserRole).toString();
    if (feedId.isEmpty())
      return;
    const auto f = service_->findFeed(feedId);
    if (!f.has_value())
      return;

    menu.addAction(QStringLiteral("刷新此订阅"), this, [this, feedId]() {
      service_->refreshFeed(feedId);
      service_->saveState();
      refreshTable();
    });
    menu.addSeparator();
    menu.addAction(QStringLiteral("复制订阅链接"), this, [u = f->url]() {
      if (!u.isEmpty()) {
        QApplication::clipboard()->setText(u);
      }
    });
    menu.addAction(QStringLiteral("复制订阅 ID"), this, [id = f->id]() {
      if (!id.isEmpty()) {
        QApplication::clipboard()->setText(id);
      }
    });
    menu.addAction(QStringLiteral("在浏览器中打开"), this, [u = f->url]() {
      if (!u.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromUserInput(u));
      }
    });
    menu.addSeparator();
    menu.addAction(QStringLiteral("编辑名称与地址…"), this,
                   [this, feedId]() { editFeedById(feedId); });
    menu.addAction(f->enabled ? QStringLiteral("暂停此订阅") : QStringLiteral("启用此订阅"), this,
                   [this, feed = *f]() mutable {
                     feed.enabled = !feed.enabled;
                     service_->upsertFeed(feed);
                     service_->saveState();
                     refreshTable();
                   });
    menu.addAction(f->auto_download_enabled ? QStringLiteral("关闭此订阅自动下载")
                                            : QStringLiteral("开启此订阅自动下载"),
                   this, [this, feed = *f]() mutable {
                     feed.auto_download_enabled = !feed.auto_download_enabled;
                     service_->upsertFeed(feed);
                     service_->saveState();
                     refreshTable();
                   });
    if (!f->last_error.isEmpty()) {
      menu.addAction(QStringLiteral("清除错误信息"), this, [this, feed = *f]() mutable {
        feed.last_error.clear();
        service_->upsertFeed(feed);
        service_->saveState();
        refreshTable();
      });
    }
    menu.addSeparator();
    menu.addAction(QStringLiteral("清空本地条目（不删订阅）…"), this, [this, feedId]() {
      const auto r = QMessageBox::question(
          this, QStringLiteral("清空本地条目"),
          QStringLiteral("将删除该订阅源下已缓存的所有条目，订阅地址本身保留。\n"
                         "之后再次刷新订阅时，若 RSS 仍包含相同条目，会重新出现在列表中。\n\n"
                         "确定继续？"),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (r != QMessageBox::Yes)
        return;
      service_->clearItemsForFeed(feedId);
      service_->saveState();
      refreshTable();
    });
    menu.addAction(QStringLiteral("删除此订阅…"), this, [this, feedId]() {
      const auto r =
          QMessageBox::question(this, QStringLiteral("删除订阅"),
                                QStringLiteral("确定删除该订阅及其条目吗？此操作不可撤销。"),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (r != QMessageBox::Yes)
        return;
      removeFeedsByIds({feedId});
    });
  } else {
    const QStringList ids = selectedFeedIds();
    menu.addAction(QStringLiteral("刷新选中订阅（%1）").arg(ids.size()), this, [this, ids]() {
      for (const QString& id : ids) {
        service_->refreshFeed(id);
      }
      service_->saveState();
      refreshTable();
    });
    menu.addSeparator();
    menu.addAction(QStringLiteral("清空选中订阅的本地条目…"), this, [this, ids]() {
      const auto r = QMessageBox::question(
          this, QStringLiteral("清空本地条目"),
          QStringLiteral("将删除选中 %1 个订阅源下已缓存的条目，订阅源保留。\n"
                         "再次刷新后条目可能重新出现。\n\n"
                         "确定继续？")
              .arg(ids.size()),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (r != QMessageBox::Yes)
        return;
      service_->clearItemsForFeeds(ids);
      service_->saveState();
      refreshTable();
    });
    menu.addAction(QStringLiteral("删除选中订阅…"), this, [this, ids]() {
      const auto r = QMessageBox::question(
          this, QStringLiteral("删除订阅"),
          QStringLiteral("确定删除选中的 %1 个订阅及其条目吗？此操作不可撤销。").arg(ids.size()),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
      if (r != QMessageBox::Yes)
        return;
      removeFeedsByIds(ids);
    });
  }

  menu.exec(globalPos);
}

void RssFeedsPage::onAddFeed() {
  if (!service_)
    return;
  const QString url = feedUrlEdit_->text().trimmed();
  if (url.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("添加订阅"), QStringLiteral("请输入订阅地址。"));
    return;
  }

  pfd::core::rss::RssFeed feed;
  feed.url = url;
  feed.title = url;
  service_->upsertFeed(feed);
  service_->refreshFeed(service_->feeds().back().id);
  service_->saveState();
  feedUrlEdit_->clear();
  refreshTable();
}

void RssFeedsPage::onRefreshAll() {
  if (!service_)
    return;
  service_->refreshAllFeeds();
  service_->saveState();
  refreshTable();
}

void RssFeedsPage::onRemoveSelected() {
  if (!service_)
    return;
  QStringList ids = selectedFeedIds();
  if (ids.isEmpty()) {
    const int row = feedTable_->currentRow();
    if (row < 0)
      return;
    const QString feedId = feedTable_->item(row, 0)->data(Qt::UserRole).toString();
    if (feedId.isEmpty())
      return;
    ids.append(feedId);
  }

  const auto r = QMessageBox::question(
      this, QStringLiteral("删除订阅"),
      ids.size() > 1 ? QStringLiteral("确定删除选中的 %1 个订阅及其条目吗？").arg(ids.size())
                     : QStringLiteral("确定删除该订阅及其条目吗？"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (r != QMessageBox::Yes)
    return;

  removeFeedsByIds(ids);
}

void RssFeedsPage::onImportOpml() {
  if (!service_)
    return;
  const QString path =
      QFileDialog::getOpenFileName(this, QStringLiteral("导入 OPML"), QString(),
                                   QStringLiteral("OPML 文件 (*.opml *.xml);;所有文件 (*)"));
  if (path.isEmpty())
    return;

  const auto result = pfd::core::rss::RssOpml::importFromFile(path);
  if (!result.ok()) {
    QMessageBox::warning(this, QStringLiteral("导入 OPML"), result.error);
    return;
  }

  int added = 0;
  for (const auto& feed : result.feeds) {
    bool exists = false;
    for (const auto& existing : service_->feeds()) {
      if (existing.url == feed.url) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      service_->upsertFeed(feed);
      ++added;
    }
  }
  service_->saveState();
  refreshTable();

  QMessageBox::information(this, QStringLiteral("导入 OPML"),
                           QStringLiteral("导入完成：新增 %1 个订阅，跳过 %2 个已有，%3 个无效。")
                               .arg(added)
                               .arg(static_cast<int>(result.feeds.size()) - added)
                               .arg(result.skipped));
}

void RssFeedsPage::onExportOpml() {
  if (!service_)
    return;
  if (service_->feeds().empty()) {
    QMessageBox::information(this, QStringLiteral("导出 OPML"),
                             QStringLiteral("暂无订阅源可导出。"));
    return;
  }

  const QString path =
      QFileDialog::getSaveFileName(this, QStringLiteral("导出 OPML"), QStringLiteral("feeds.opml"),
                                   QStringLiteral("OPML 文件 (*.opml);;所有文件 (*)"));
  if (path.isEmpty())
    return;

  if (pfd::core::rss::RssOpml::exportToFile(path, service_->feeds())) {
    QMessageBox::information(this, QStringLiteral("导出 OPML"),
                             QStringLiteral("已导出 %1 个订阅源。").arg(service_->feeds().size()));
  } else {
    QMessageBox::warning(this, QStringLiteral("导出 OPML"), QStringLiteral("写入文件失败。"));
  }
}

}  // namespace pfd::ui::rss
