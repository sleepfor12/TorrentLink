#ifndef PFD_UI_WIDGETS_BOTTOM_STATUS_BAR_H
#define PFD_UI_WIDGETS_BOTTOM_STATUS_BAR_H

#include <QtWidgets/QWidget>

class QLabel;

namespace pfd::ui {

class BottomStatusBar final : public QWidget {
  Q_OBJECT

public:
  explicit BottomStatusBar(QWidget* parent = nullptr);

  void setStatus(int dhtNodes, qint64 downloadRateBytesPerSec, qint64 uploadRateBytesPerSec);

private:
  QLabel* dht_{nullptr};
  QLabel* dl_{nullptr};
  QLabel* ul_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_WIDGETS_BOTTOM_STATUS_BAR_H
