#ifndef PFD_UI_RSS_RSS_MODULE_PAGE_H
#define PFD_UI_RSS_RSS_MODULE_PAGE_H

#include <QtWidgets/QWidget>

#include <functional>
#include <vector>

#include "core/task_snapshot.h"

class QTabWidget;

namespace pfd::core::rss {
class RssService;
}

namespace pfd::ui::rss {

class RssFeedsPage;
class RssItemsPage;
class RssRulesPage;

class RssModulePage : public QWidget {
  Q_OBJECT

public:
  explicit RssModulePage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);

  void setItemsPageUiHelpers(std::function<void(const QString&)> appendItemsLog,
                             std::function<QString()> defaultSaveRoot,
                             std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots);

  void refreshDataViews();
  void refreshItemsTaskProgress();

Q_SIGNALS:
  void dataViewsRefreshed();
  void rssNetworkRefreshFinished();

private:
  void buildLayout();

  QTabWidget* tabs_{nullptr};
  RssFeedsPage* feedsPage_{nullptr};
  RssItemsPage* itemsPage_{nullptr};
  RssRulesPage* rulesPage_{nullptr};
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_MODULE_PAGE_H
