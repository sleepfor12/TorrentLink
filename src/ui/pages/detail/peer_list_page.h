#ifndef PFD_UI_PAGES_DETAIL_PEER_LIST_PAGE_H
#define PFD_UI_PAGES_DETAIL_PEER_LIST_PAGE_H

#include <QtWidgets/QWidget>

#include <functional>

#include "core/task_query_dto.h"
#include "core/task_snapshot.h"

class QTableWidget;
class QTimer;

namespace pfd::ui {

class PeerListPage : public QWidget {
  Q_OBJECT

public:
  using QueryPeersFn = std::function<std::vector<pfd::core::TaskPeerDto>(const pfd::base::TaskId&)>;

  explicit PeerListPage(QWidget* parent = nullptr);

  void setQueryFn(QueryPeersFn fn);
  void setSnapshot(const pfd::core::TaskSnapshot& snap);
  void clear();

protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

private:
  void buildLayout();
  void reload();

  QueryPeersFn queryFn_;
  pfd::base::TaskId taskId_;
  QTableWidget* table_{nullptr};
  QTimer* refreshTimer_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_PEER_LIST_PAGE_H
