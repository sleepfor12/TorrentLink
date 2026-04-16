#ifndef PFD_UI_RSS_RSS_FEEDS_PAGE_H
#define PFD_UI_RSS_RSS_FEEDS_PAGE_H

#include <QtWidgets/QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;

namespace pfd::core::rss {
class RssService;
}

namespace pfd::ui::rss {

class RssFeedsPage : public QWidget {
  Q_OBJECT

public:
  explicit RssFeedsPage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);
  void refreshTable();

private slots:
  void showFeedContextMenu(const QPoint& pos);

private:
  void buildLayout();
  void bindSignals();
  void onAddFeed();
  void onRefreshAll();
  void onRemoveSelected();
  void onImportOpml();
  void onExportOpml();

  [[nodiscard]] QStringList selectedFeedIds() const;
  void editFeedById(const QString& feedId);
  void removeFeedsByIds(const QStringList& ids);

  pfd::core::rss::RssService* service_{nullptr};
  QLineEdit* feedUrlEdit_{nullptr};
  QPushButton* addFeedBtn_{nullptr};
  QPushButton* refreshAllBtn_{nullptr};
  QPushButton* removeBtn_{nullptr};
  QPushButton* importBtn_{nullptr};
  QPushButton* exportBtn_{nullptr};
  QTableWidget* feedTable_{nullptr};
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_FEEDS_PAGE_H
