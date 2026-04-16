#include "ui/rss/rss_module_page.h"

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include "ui/rss/rss_feeds_page.h"
#include "ui/rss/rss_items_page.h"
#include "ui/rss/rss_rules_page.h"
#include "ui/rss/rss_series_page.h"

namespace pfd::ui::rss {

RssModulePage::RssModulePage(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void RssModulePage::setService(pfd::core::rss::RssService* service) {
  feedsPage_->setService(service);
  itemsPage_->setService(service);
  rulesPage_->setService(service);
  seriesPage_->setService(service);
}

void RssModulePage::setItemsPageUiHelpers(
    std::function<void(const QString&)> appendItemsLog, std::function<QString()> defaultSaveRoot,
    std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots) {
  if (itemsPage_) {
    itemsPage_->setUiHelpers(std::move(appendItemsLog), std::move(defaultSaveRoot),
                             std::move(taskSnapshots));
  }
}

void RssModulePage::refreshDataViews() {
  if (feedsPage_) {
    feedsPage_->refreshTable();
  }
  if (itemsPage_) {
    itemsPage_->refreshTable();
  }
  if (rulesPage_) {
    rulesPage_->refreshTable();
  }
  if (seriesPage_) {
    seriesPage_->refreshTable();
  }
}

void RssModulePage::refreshItemsTaskProgress() {
  if (itemsPage_) {
    itemsPage_->refreshTaskProgressCells();
  }
}

void RssModulePage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  tabs_ = new QTabWidget(this);
  feedsPage_ = new RssFeedsPage(tabs_);
  itemsPage_ = new RssItemsPage(tabs_);
  rulesPage_ = new RssRulesPage(tabs_);
  seriesPage_ = new RssSeriesPage(tabs_);
  tabs_->addTab(feedsPage_, QStringLiteral("订阅源"));
  tabs_->addTab(itemsPage_, QStringLiteral("条目流"));
  tabs_->addTab(rulesPage_, QStringLiteral("自动规则"));
  tabs_->addTab(seriesPage_, QStringLiteral("剧集订阅"));
  root->addWidget(tabs_);
}

}  // namespace pfd::ui::rss
