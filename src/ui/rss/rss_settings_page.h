#ifndef PFD_UI_RSS_RSS_SETTINGS_PAGE_H
#define PFD_UI_RSS_RSS_SETTINGS_PAGE_H

#include <QtWidgets/QWidget>

class QCheckBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace pfd::core::rss {
class RssService;
}

namespace pfd::ui::rss {

class RssSettingsPage : public QWidget {
  Q_OBJECT

public:
  explicit RssSettingsPage(QWidget* parent = nullptr);

  void setService(pfd::core::rss::RssService* service);

Q_SIGNALS:
  void settingsSaved();

private:
  void buildLayout();
  void loadFromService();
  void onSave();

  pfd::core::rss::RssService* service_{nullptr};

  QCheckBox* globalAutoDownloadCheck_{nullptr};
  QSpinBox* refreshIntervalSpin_{nullptr};
  QSpinBox* maxAutoDownloadsSpin_{nullptr};
  QSpinBox* historyMaxItemsSpin_{nullptr};
  QSpinBox* historyMaxAgeSpin_{nullptr};
  QLineEdit* playerCommandEdit_{nullptr};
  QPushButton* saveBtn_{nullptr};
};

}  // namespace pfd::ui::rss

#endif  // PFD_UI_RSS_RSS_SETTINGS_PAGE_H
