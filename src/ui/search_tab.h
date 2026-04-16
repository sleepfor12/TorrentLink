#ifndef PFD_UI_SEARCH_TAB_H
#define PFD_UI_SEARCH_TAB_H

#include <QtWidgets/QWidget>

#include <functional>
#include <vector>

#include "core/task_snapshot.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QListWidget;
class QTextBrowser;

namespace pfd::ui {

class SearchTab : public QWidget {
  Q_OBJECT

public:
  using QuerySnapshotsFn = std::function<std::vector<pfd::core::TaskSnapshot>()>;
  using QueryRssItemsFn = std::function<std::vector<std::pair<QString, QString>>(const QString&)>;

  explicit SearchTab(QWidget* parent = nullptr);

  void setQuerySnapshotsFn(QuerySnapshotsFn fn);
  void setQueryRssItemsFn(QueryRssItemsFn fn);

private:
  void buildLayout();
  void performSearch();
  void onResultSelected(int row);

  QLineEdit* queryEdit_{nullptr};
  QComboBox* scopeBox_{nullptr};
  QPushButton* searchBtn_{nullptr};
  QListWidget* resultList_{nullptr};
  QTextBrowser* resultView_{nullptr};

  QuerySnapshotsFn querySnapshotsFn_;
  QueryRssItemsFn queryRssItemsFn_;

  struct ResultEntry {
    QString title;
    QString detail;
  };
  std::vector<ResultEntry> results_;
};

}  // namespace pfd::ui

#endif  // PFD_UI_SEARCH_TAB_H
