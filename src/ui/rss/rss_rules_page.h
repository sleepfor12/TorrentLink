#ifndef PFD_UI_RSS_RSS_RULES_PAGE_H
#define PFD_UI_RSS_RSS_RULES_PAGE_H

#include <QtWidgets/QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace pfd::core::rss {
class RssService;
}

namespace pfd::ui::rss {

class RssRulesPage : public QWidget {
  Q_OBJECT

public:
  explicit RssRulesPage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);
  void refreshTable();

private:
  void buildLayout();
  void bindSignals();
  void onAddRule();
  void onEditSelected();
  void onRemoveSelected();
  void onToggleEnabled();
  void onSelectionChanged();
  void updateSwitchStatus();

  pfd::core::rss::RssService* service_{nullptr};

  QLabel* globalSwitch_{nullptr};
  QPushButton* addBtn_{nullptr};
  QPushButton* editBtn_{nullptr};
  QPushButton* removeBtn_{nullptr};
  QPushButton* toggleBtn_{nullptr};
  QTableWidget* ruleTable_{nullptr};

  QLineEdit* nameEdit_{nullptr};
  QLineEdit* feedIdsEdit_{nullptr};
  QLineEdit* includeEdit_{nullptr};
  QLineEdit* excludeEdit_{nullptr};
  QCheckBox* regexCheck_{nullptr};
  QLineEdit* savePathEdit_{nullptr};
  QPushButton* browseSavePathBtn_{nullptr};
  QLineEdit* categoryEdit_{nullptr};
  QLineEdit* tagsEdit_{nullptr};
  QPushButton* saveFormBtn_{nullptr};
  QWidget* formContainer_{nullptr};

  QString editingRuleId_;
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_RULES_PAGE_H
