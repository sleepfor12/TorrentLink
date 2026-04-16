#ifndef PFD_UI_PAGES_DETAIL_GENERAL_DETAIL_PAGE_H
#define PFD_UI_PAGES_DETAIL_GENERAL_DETAIL_PAGE_H

#include <QtWidgets/QWidget>

#include "core/task_snapshot.h"

class QLabel;
class QProgressBar;

namespace pfd::ui {

class GeneralDetailPage : public QWidget {
  Q_OBJECT

public:
  explicit GeneralDetailPage(QWidget* parent = nullptr);

  void setSnapshot(const pfd::core::TaskSnapshot& snap);
  void clear();

private:
  void buildLayout();

  // Progress
  QProgressBar* progressBar_{nullptr};
  QLabel* progressLabel_{nullptr};

  // Transfer
  QLabel* activeTime_{nullptr};
  QLabel* eta_{nullptr};
  QLabel* connections_{nullptr};
  QLabel* downloaded_{nullptr};
  QLabel* uploaded_{nullptr};
  QLabel* seeds_{nullptr};
  QLabel* dlSpeed_{nullptr};
  QLabel* ulSpeed_{nullptr};
  QLabel* peers_{nullptr};
  QLabel* dlLimit_{nullptr};
  QLabel* ulLimit_{nullptr};
  QLabel* wasted_{nullptr};
  QLabel* shareRatio_{nullptr};
  QLabel* nextAnnounce_{nullptr};
  QLabel* lastSeenComplete_{nullptr};
  QLabel* popularity_{nullptr};

  // Info
  QLabel* totalSize_{nullptr};
  QLabel* pieces_{nullptr};
  QLabel* createdBy_{nullptr};
  QLabel* addedOn_{nullptr};
  QLabel* completedOn_{nullptr};
  QLabel* createdOn_{nullptr};
  QLabel* isPrivate_{nullptr};
  QLabel* infoHashV1_{nullptr};
  QLabel* infoHashV2_{nullptr};
  QLabel* savePath_{nullptr};
  QLabel* comment_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_GENERAL_DETAIL_PAGE_H
