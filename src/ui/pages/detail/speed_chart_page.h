#ifndef PFD_UI_PAGES_DETAIL_SPEED_CHART_PAGE_H
#define PFD_UI_PAGES_DETAIL_SPEED_CHART_PAGE_H

#include <QtCore/QtGlobal>
#include <QtWidgets/QWidget>

class QQuickWidget;

namespace pfd::ui {

class SpeedChartModel;

class SpeedChartPage : public QWidget {
  Q_OBJECT

public:
  explicit SpeedChartPage(QWidget* parent = nullptr);

  void addSample(qint64 downloadRate, qint64 uploadRate);
  void clear();

private:
  SpeedChartModel* model_{nullptr};
  QQuickWidget* qmlView_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_PAGES_DETAIL_SPEED_CHART_PAGE_H
