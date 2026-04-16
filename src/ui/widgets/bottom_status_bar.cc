#include "ui/widgets/bottom_status_bar.h"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

#include "base/format.h"

namespace pfd::ui {

BottomStatusBar::BottomStatusBar(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("BottomStatusBar"));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 8, 12, 8);
  layout->setSpacing(16);

  dht_ = new QLabel(QStringLiteral("DHT 节点：--"), this);
  dht_->setObjectName(QStringLiteral("BottomStatusLabel"));
  dl_ = new QLabel(QStringLiteral("↓ --"), this);
  dl_->setObjectName(QStringLiteral("BottomStatusLabel"));
  ul_ = new QLabel(QStringLiteral("↑ --"), this);
  ul_->setObjectName(QStringLiteral("BottomStatusLabel"));

  layout->addWidget(dht_);
  layout->addStretch(1);
  layout->addWidget(dl_);
  layout->addWidget(ul_);
}

void BottomStatusBar::setStatus(int dhtNodes, qint64 downloadRateBytesPerSec,
                                qint64 uploadRateBytesPerSec) {
  if (dht_ != nullptr) {
    dht_->setText(QStringLiteral("DHT 节点：%1")
                      .arg(dhtNodes >= 0 ? QString::number(dhtNodes) : QStringLiteral("--")));
  }
  if (dl_ != nullptr) {
    dl_->setText(QStringLiteral("↓ %1").arg(pfd::base::formatRate(downloadRateBytesPerSec)));
  }
  if (ul_ != nullptr) {
    ul_->setText(QStringLiteral("↑ %1").arg(pfd::base::formatRate(uploadRateBytesPerSec)));
  }
}

}  // namespace pfd::ui
