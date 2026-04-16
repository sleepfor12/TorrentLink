#include "ui/pages/detail/http_source_page.h"

#include <QtCore/QTimer>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

namespace pfd::ui {

HttpSourcePage::HttpSourcePage(QWidget* parent) : QWidget(parent) { buildLayout(); }

void HttpSourcePage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);

  table_ = new QTableWidget(this);
  table_->setColumnCount(2);
  table_->setHorizontalHeaderLabels({QStringLiteral("URL"), QStringLiteral("类型")});
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->setVisible(false);
  table_->horizontalHeader()->setStretchLastSection(true);
  table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table_->setAlternatingRowColors(true);
  root->addWidget(table_);

  refreshTimer_ = new QTimer(this);
  refreshTimer_->setInterval(5000);
  connect(refreshTimer_, &QTimer::timeout, this, &HttpSourcePage::reload);
}

void HttpSourcePage::setQueryFn(QueryWebSeedsFn fn) { queryFn_ = std::move(fn); }

void HttpSourcePage::setSnapshot(const pfd::core::TaskSnapshot& snap) {
  taskId_ = snap.taskId;
  if (isVisible()) {
    reload();
  }
}

void HttpSourcePage::clear() {
  taskId_ = {};
  table_->setRowCount(0);
}

void HttpSourcePage::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  reload();
  refreshTimer_->start();
}

void HttpSourcePage::hideEvent(QHideEvent* event) {
  QWidget::hideEvent(event);
  refreshTimer_->stop();
}

void HttpSourcePage::reload() {
  if (!queryFn_ || taskId_.isNull()) return;
  const auto seeds = queryFn_(taskId_);
  table_->setRowCount(static_cast<int>(seeds.size()));
  for (int i = 0; i < static_cast<int>(seeds.size()); ++i) {
    const auto& s = seeds[static_cast<size_t>(i)];
    auto makeItem = [](const QString& text) {
      auto* item = new QTableWidgetItem(text);
      item->setFlags(item->flags() & ~Qt::ItemIsEditable);
      return item;
    };
    table_->setItem(i, 0, makeItem(s.url));
    table_->setItem(i, 1, makeItem(s.type));
  }
}

}  // namespace pfd::ui
