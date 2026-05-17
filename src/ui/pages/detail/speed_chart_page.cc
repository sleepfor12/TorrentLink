#include "ui/pages/detail/speed_chart_page.h"

#include <QtCore/QMetaObject>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>
#include <QtQuickWidgets/QQuickWidget>
#include <QtWidgets/QVBoxLayout>

#include "core/config_service.h"
#include "ui/app_theme.h"
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
  pushThemeTokensToQml();

  layout->addWidget(qmlView_);
}

void SpeedChartPage::pushThemeTokensToQml() {
  if (qmlView_ == nullptr) {
    return;
  }
  const auto st = pfd::core::ConfigService::loadAppSettings();
  const EffectiveUiTheme eff = UiTheme::resolveEffectiveTheme(st.ui_theme);
  const QVariantMap tokens = UiTheme::speedChartThemeTokens(eff);
  QQuickItem* rootItem = qmlView_->rootObject();
  if (rootItem == nullptr) {
    return;
  }
  QObject* root = rootItem;
  for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it) {
    root->setProperty(it.key().toUtf8().constData(), it.value());
  }
  if (QObject* canvas = root->findChild<QObject*>(QStringLiteral("speedChartCanvas"))) {
    QMetaObject::invokeMethod(canvas, "requestPaint", Qt::QueuedConnection);
  }
}

void SpeedChartPage::syncChartTheme() {
  pushThemeTokensToQml();
}

void SpeedChartPage::addSample(qint64 downloadRate, qint64 uploadRate) {
  model_->addSample(downloadRate, uploadRate);
}

void SpeedChartPage::clear() {
  model_->clear();
}

}  // namespace pfd::ui
