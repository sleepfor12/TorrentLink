#ifndef PFD_UI_RSS_RSS_SERIES_PAGE_H
#define PFD_UI_RSS_RSS_SERIES_PAGE_H

#include <QtWidgets/QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace pfd::core::rss {
class RssService;
}

namespace pfd::ui::rss {

class RssSeriesPage : public QWidget {
  Q_OBJECT

public:
  explicit RssSeriesPage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);
  void refreshTable();

private:
  void buildLayout();
  void bindSignals();
  void onAddSeries();
  void onEditSelected();
  void onRemoveSelected();
  void onToggleEnabled();

  pfd::core::rss::RssService* service_{nullptr};

  QPushButton* addBtn_{nullptr};
  QPushButton* editBtn_{nullptr};
  QPushButton* removeBtn_{nullptr};
  QPushButton* toggleBtn_{nullptr};
  QTableWidget* seriesTable_{nullptr};

  QLineEdit* nameEdit_{nullptr};
  QLineEdit* aliasesEdit_{nullptr};
  QLineEdit* qualityEdit_{nullptr};
  QSpinBox* seasonSpin_{nullptr};
  QSpinBox* lastEpSpin_{nullptr};
  QCheckBox* autoDownloadCheck_{nullptr};
  QLineEdit* savePathEdit_{nullptr};
  QPushButton* browseSavePathBtn_{nullptr};
  QPushButton* saveFormBtn_{nullptr};
  QWidget* formContainer_{nullptr};

  QString editingSeriesId_;
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_SERIES_PAGE_H
