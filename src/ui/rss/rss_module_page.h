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
class RssSeriesPage;

class RssModulePage : public QWidget {
  Q_OBJECT

public:
  explicit RssModulePage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);

  void setItemsPageUiHelpers(std::function<void(const QString&)> appendItemsLog,
                             std::function<QString()> defaultSaveRoot,
                             std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots);

  /// 条目下载状态、剧集订阅进度等变更后刷新相关子页表格。
  void refreshDataViews();
  void refreshItemsTaskProgress();

private:
  void buildLayout();

  QTabWidget* tabs_{nullptr};
  RssFeedsPage* feedsPage_{nullptr};
  RssItemsPage* itemsPage_{nullptr};
  RssRulesPage* rulesPage_{nullptr};
  RssSeriesPage* seriesPage_{nullptr};
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_MODULE_PAGE_H
