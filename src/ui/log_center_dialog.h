#ifndef PFD_UI_LOG_CENTER_DIALOG_H
#define PFD_UI_LOG_CENTER_DIALOG_H

#include <QtCore/QStringList>
#include <QtWidgets/QDialog>

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

namespace pfd::ui {

class LogCenterDialog : public QDialog {
  Q_OBJECT

public:
  explicit LogCenterDialog(QWidget* parent = nullptr);

private:
  void buildLayout();
  void bindSignals();
  void reloadLogs();
  [[nodiscard]] QStringList buildFilteredLines() const;
  void exportLogs();
  void clearLogs();
  void openLogDir();

  QComboBox* levelFilter_{nullptr};
  QComboBox* maxRowsFilter_{nullptr};
  QCheckBox* newestFirstCheck_{nullptr};
  QLineEdit* keywordFilter_{nullptr};
  QLineEdit* rssFeedFilter_{nullptr};
  QLineEdit* rssItemFilter_{nullptr};
  QLineEdit* rssRuleFilter_{nullptr};
  QPlainTextEdit* contentView_{nullptr};
  QPushButton* refreshBtn_{nullptr};
  QPushButton* exportBtn_{nullptr};
  QPushButton* clearBtn_{nullptr};
  QPushButton* openDirBtn_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_LOG_CENTER_DIALOG_H
