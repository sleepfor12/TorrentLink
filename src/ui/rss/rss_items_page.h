#ifndef PFD_UI_RSS_RSS_ITEMS_PAGE_H
#define PFD_UI_RSS_RSS_ITEMS_PAGE_H

#include <QtCore/QEvent>
#include <QtCore/QStringList>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QWidget>

#include <functional>
#include <vector>

#include "core/task_snapshot.h"

class QLineEdit;
class QPushButton;
class QTableWidget;
class QTextBrowser;
namespace pfd::core::rss {
class RssService;
struct RssItem;
}  // namespace pfd::core::rss

namespace pfd::ui::rss {

class RssItemsPage : public QWidget {
  Q_OBJECT

public:
  explicit RssItemsPage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);
  void setUiHelpers(std::function<void(const QString&)> appendItemsLog,
                    std::function<QString()> defaultSaveRoot,
                    std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots);
  void refreshTable();
  /// 仅更新「任务进度」列（与传输列表快照对齐）；由 MainWindow::refreshTasks 周期性调用。
  void refreshTaskProgressCells();

protected:
  void keyPressEvent(QKeyEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  void buildLayout();
  void bindSignals();
  void updateActionStates();
  void onSelectionChanged();
  void onDownloadClicked();
  void onFilterChanged();
  void onCopyMagnet();
  void onOpenFolder();
  void onPlayClicked();
  void onIgnoreItem();

  [[nodiscard]] QString resolveSelectedItemPath(const QString& actionName) const;
  void showRowContextMenu(const QPoint& pos);

  [[nodiscard]] QStringList selectedItemIdsOrdered() const;
  [[nodiscard]] const pfd::core::rss::RssItem* findItem(const QString& id) const;
  [[nodiscard]] bool selectionHasDownloadable() const;
  void userLog(const QString& msg) const;

  pfd::core::rss::RssService* service_{nullptr};
  std::function<void(const QString&)> appendItemsLog_;
  std::function<QString()> defaultSaveRoot_;
  std::function<std::vector<pfd::core::TaskSnapshot>()> taskSnapshots_;

  QLineEdit* searchEdit_{nullptr};
  QPushButton* downloadBtn_{nullptr};
  QPushButton* refreshBtn_{nullptr};
  QPushButton* copyMagnetBtn_{nullptr};
  QPushButton* openFolderBtn_{nullptr};
  QPushButton* playBtn_{nullptr};
  QPushButton* ignoreBtn_{nullptr};
  QTableWidget* itemTable_{nullptr};
  QTextBrowser* detailView_{nullptr};
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_ITEMS_PAGE_H
