#ifndef PFD_UI_PAGES_DETAIL_CONTENT_TREE_PAGE_H
#define PFD_UI_PAGES_DETAIL_CONTENT_TREE_PAGE_H

#include <QtWidgets/QWidget>

#include <functional>
#include <vector>

#include "core/task_query_dto.h"
#include "core/task_snapshot.h"

class QTreeWidget;
class QLineEdit;
class QPushButton;
class QTreeWidgetItem;

namespace pfd::ui {

class ContentTreePage : public QWidget {
  Q_OBJECT

public:
  using QueryFilesFn = std::function<std::vector<pfd::core::TaskFileEntryDto>(const pfd::base::TaskId&)>;
  using SetPriorityFn = std::function<void(const pfd::base::TaskId&, const std::vector<int>&,
                                           pfd::core::TaskFilePriorityLevel)>;
  using RenameFn = std::function<void(const pfd::base::TaskId&, const QString&, const QString&)>;

  explicit ContentTreePage(QWidget* parent = nullptr);
  void setHandlers(QueryFilesFn queryFn, SetPriorityFn priorityFn, RenameFn renameFn);
  void setSnapshot(const pfd::core::TaskSnapshot& snap);
  void clear();

private:
  void buildLayout();
  void reloadTree();
  void applySearchFilter();
  void showContextMenu(const QPoint& pos);
  void applyPriorityToSelection(pfd::core::TaskFilePriorityLevel level);
  std::vector<int> collectFileIndices(QTreeWidgetItem* item) const;
  QString logicalPathForItem(QTreeWidgetItem* item) const;
  void applyRenameLocally(QTreeWidgetItem* item, const QString& newName);
  void rewritePathRecursive(QTreeWidgetItem* item, const QString& oldPrefix,
                            const QString& newPrefix);

  QueryFilesFn queryFilesFn_;
  SetPriorityFn setPriorityFn_;
  RenameFn renameFn_;
  pfd::base::TaskId taskId_;
  QString savePath_;

  QLineEdit* searchEdit_{nullptr};
  QPushButton* selectAllBtn_{nullptr};
  QPushButton* selectNoneBtn_{nullptr};
  QTreeWidget* tree_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_CONTENT_TREE_PAGE_H
