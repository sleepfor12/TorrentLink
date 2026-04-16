#include "ui/pages/detail/speed_chart_page.h"

#include <QtQml/QQmlContext>
#include <QtQuickWidgets/QQuickWidget>
#include <QtWidgets/QVBoxLayout>

#include "ui/pages/detail/speed_chart_model.h"

void ensureDetailQmlResourceLoaded() {
  Q_INIT_RESOURCE(detail_qml);
}

namespace pfd::ui {

SpeedChartPage::SpeedChartPage(QWidget* parent) : QWidget(parent) {
  setMinimumHeight(120);
  ensureDetailQmlResourceLoaded();

  model_ = new SpeedChartModel(this);
  // Seed one sample so chart has immediate baseline.
  model_->addSample(0, 0);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  qmlView_ = new QQuickWidget(this);
  qmlView_->setResizeMode(QQuickWidget::SizeRootObjectToView);
  qmlView_->rootContext()->setContextProperty(QStringLiteral("chartModelRef"), model_);
  qmlView_->setSource(QUrl(QStringLiteral("qrc:/detail/SpeedChart.qml")));

  layout->addWidget(qmlView_);
}

void SpeedChartPage::addSample(qint64 downloadRate, qint64 uploadRate) {
  model_->addSample(downloadRate, uploadRate);
}

void SpeedChartPage::clear() {
  model_->clear();
}

}  // namespace pfd::ui
