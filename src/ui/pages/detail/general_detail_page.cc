#include "ui/pages/detail/general_detail_page.h"

#include <QtCore/QDateTime>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>

#include "base/format.h"

namespace pfd::ui {

namespace {

const QString kDash = QStringLiteral("--");

QLabel* makeValueLabel(QWidget* parent) {
  auto* l = new QLabel(kDash, parent);
  l->setTextInteractionFlags(Qt::TextSelectableByMouse);
  l->setStyleSheet(QStringLiteral("color:#1f2d3d;font-size:12px;"));
  return l;
}

QGroupBox* makeGroup(const QString& title, QWidget* parent) {
  auto* g = new QGroupBox(title, parent);
  g->setStyleSheet(
      QStringLiteral("QGroupBox{font-weight:700;font-size:12px;color:#374151;"
                     "border:1px solid #e5e7eb;border-radius:4px;margin-top:8px;padding-top:14px;}"
                     "QGroupBox::title{subcontrol-origin:margin;left:8px;padding:0 4px;}"));
  return g;
}

void addRow(QFormLayout* form, const QString& label, QLabel* value) {
  auto* lbl = new QLabel(label);
  lbl->setStyleSheet(QStringLiteral("color:#6b7280;font-size:12px;"));
  lbl->setFixedWidth(110);
  form->addRow(lbl, value);
}

}  // namespace

GeneralDetailPage::GeneralDetailPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void GeneralDetailPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 4, 8, 4);
  root->setSpacing(4);

  // --- Progress ---
  auto* progressGroup = makeGroup(QStringLiteral("进度"), this);
  auto* pLayout = new QVBoxLayout(progressGroup);
  pLayout->setContentsMargins(8, 4, 8, 4);
  progressBar_ = new QProgressBar(progressGroup);
  progressBar_->setRange(0, 1000);
  progressBar_->setValue(0);
  progressBar_->setTextVisible(true);
  progressBar_->setFixedHeight(20);
  progressLabel_ = new QLabel(QStringLiteral("0.0%"), progressGroup);
  progressLabel_->setAlignment(Qt::AlignRight);
  progressLabel_->setStyleSheet(QStringLiteral("font-size:11px;color:#374151;"));
  pLayout->addWidget(progressBar_);
  pLayout->addWidget(progressLabel_);
  root->addWidget(progressGroup);

  // --- Transfer ---
  auto* transferGroup = makeGroup(QStringLiteral("传输"), this);

  auto* row1 = new QHBoxLayout();
  auto* col1 = new QFormLayout();
  auto* col2 = new QFormLayout();
  auto* col3 = new QFormLayout();
  for (auto* c : {col1, col2, col3}) {
    c->setHorizontalSpacing(8);
    c->setVerticalSpacing(2);
  }

  activeTime_ = makeValueLabel(transferGroup);
  eta_ = makeValueLabel(transferGroup);
  connections_ = makeValueLabel(transferGroup);
  downloaded_ = makeValueLabel(transferGroup);
  uploaded_ = makeValueLabel(transferGroup);
  seeds_ = makeValueLabel(transferGroup);
  dlSpeed_ = makeValueLabel(transferGroup);
  ulSpeed_ = makeValueLabel(transferGroup);
  peers_ = makeValueLabel(transferGroup);
  dlLimit_ = makeValueLabel(transferGroup);
  ulLimit_ = makeValueLabel(transferGroup);
  wasted_ = makeValueLabel(transferGroup);
  shareRatio_ = makeValueLabel(transferGroup);
  nextAnnounce_ = makeValueLabel(transferGroup);
  lastSeenComplete_ = makeValueLabel(transferGroup);
  popularity_ = makeValueLabel(transferGroup);

  addRow(col1, QStringLiteral("活动时间："), activeTime_);
  addRow(col1, QStringLiteral("已下载："), downloaded_);
  addRow(col1, QStringLiteral("下载速度："), dlSpeed_);
  addRow(col1, QStringLiteral("下载限制："), dlLimit_);
  addRow(col1, QStringLiteral("分享率："), shareRatio_);
  addRow(col1, QStringLiteral("流行度："), popularity_);

  addRow(col2, QStringLiteral("剩余时间："), eta_);
  addRow(col2, QStringLiteral("已上传："), uploaded_);
  addRow(col2, QStringLiteral("上传速度："), ulSpeed_);
  addRow(col2, QStringLiteral("上传限制："), ulLimit_);
  addRow(col2, QStringLiteral("下次汇报："), nextAnnounce_);

  addRow(col3, QStringLiteral("连接："), connections_);
  addRow(col3, QStringLiteral("做种数："), seeds_);
  addRow(col3, QStringLiteral("用户："), peers_);
  addRow(col3, QStringLiteral("已丢弃："), wasted_);
  addRow(col3, QStringLiteral("最后完整可见："), lastSeenComplete_);

  row1->addLayout(col1, 1);
  row1->addLayout(col2, 1);
  row1->addLayout(col3, 1);

  auto* transferInner = new QVBoxLayout();
  transferInner->setContentsMargins(0, 0, 0, 0);
  transferInner->addLayout(row1);
  transferGroup->setLayout(transferInner);
  root->addWidget(transferGroup);

  // --- Info ---
  auto* infoGroup = makeGroup(QStringLiteral("信息"), this);
  auto* infoOuter = new QHBoxLayout();
  auto* ic1 = new QFormLayout();
  auto* ic2 = new QFormLayout();
  auto* ic3 = new QFormLayout();
  for (auto* c : {ic1, ic2, ic3}) {
    c->setHorizontalSpacing(8);
    c->setVerticalSpacing(2);
  }

  totalSize_ = makeValueLabel(infoGroup);
  pieces_ = makeValueLabel(infoGroup);
  createdBy_ = makeValueLabel(infoGroup);
  addedOn_ = makeValueLabel(infoGroup);
  completedOn_ = makeValueLabel(infoGroup);
  createdOn_ = makeValueLabel(infoGroup);
  isPrivate_ = makeValueLabel(infoGroup);
  infoHashV1_ = makeValueLabel(infoGroup);
  infoHashV2_ = makeValueLabel(infoGroup);
  savePath_ = makeValueLabel(infoGroup);
  comment_ = makeValueLabel(infoGroup);

  addRow(ic1, QStringLiteral("总大小："), totalSize_);
  addRow(ic1, QStringLiteral("添加于："), addedOn_);
  addRow(ic1, QStringLiteral("私密："), isPrivate_);

  addRow(ic2, QStringLiteral("区块："), pieces_);
  addRow(ic2, QStringLiteral("完成于："), completedOn_);

  addRow(ic3, QStringLiteral("创建："), createdBy_);
  addRow(ic3, QStringLiteral("创建于："), createdOn_);

  infoOuter->addLayout(ic1, 1);
  infoOuter->addLayout(ic2, 1);
  infoOuter->addLayout(ic3, 1);

  auto* infoVBox = new QVBoxLayout();
  infoVBox->setContentsMargins(0, 0, 0, 0);
  infoVBox->addLayout(infoOuter);

  auto* hashForm = new QFormLayout();
  hashForm->setHorizontalSpacing(8);
  hashForm->setVerticalSpacing(2);
  addRow(hashForm, QStringLiteral("信息哈希 v1："), infoHashV1_);
  addRow(hashForm, QStringLiteral("信息哈希 v2："), infoHashV2_);
  addRow(hashForm, QStringLiteral("保存路径："), savePath_);
  addRow(hashForm, QStringLiteral("注释："), comment_);
  infoVBox->addLayout(hashForm);

  infoGroup->setLayout(infoVBox);
  root->addWidget(infoGroup);

  root->addStretch(1);
}

void GeneralDetailPage::setSnapshot(const pfd::core::TaskSnapshot& s) {
  // Progress
  const double pct = s.progress01();
  progressBar_->setValue(static_cast<int>(pct * 1000.0));
  progressLabel_->setText(pfd::base::formatPercent(pct));

  // Transfer — live fields
  downloaded_->setText(pfd::base::formatBytes(s.downloadedBytes));
  uploaded_->setText(pfd::base::formatBytes(s.uploadedBytes));
  dlSpeed_->setText(pfd::base::formatRate(s.downloadRate));
  ulSpeed_->setText(pfd::base::formatRate(s.uploadRate));
  seeds_->setText(QString::number(s.seeders));
  peers_->setText(QString::number(s.users));
  connections_->setText(QString::number(s.seeders + s.users));

  const qint64 left = std::max<qint64>(0, s.totalBytes - s.downloadedBytes);
  eta_->setText(s.downloadRate > 0 ? pfd::base::formatDuration(left / s.downloadRate) : kDash);

  if (s.downloadedBytes > 0) {
    shareRatio_->setText(pfd::base::formatRatio(static_cast<double>(s.uploadedBytes) /
                                                static_cast<double>(s.downloadedBytes)));
  } else {
    shareRatio_->setText(kDash);
  }

  popularity_->setText(s.availability > 0.0 ? QStringLiteral("%1").arg(s.availability, 0, 'f', 2)
                                            : kDash);

  activeTime_->setText(s.activeTimeSec > 0 ? pfd::base::formatDuration(s.activeTimeSec) : kDash);
  dlLimit_->setText(s.dlLimitBps > 0 ? pfd::base::formatRate(s.dlLimitBps) : QStringLiteral("∞"));
  ulLimit_->setText(s.ulLimitBps > 0 ? pfd::base::formatRate(s.ulLimitBps) : QStringLiteral("∞"));
  wasted_->setText(s.wastedBytes > 0 ? pfd::base::formatBytes(s.wastedBytes) : kDash);
  nextAnnounce_->setText(s.nextAnnounceSec >= 0 ? pfd::base::formatDuration(s.nextAnnounceSec)
                                                : kDash);
  lastSeenComplete_->setText(pfd::base::isValidMs(s.lastSeenCompleteMs)
                                 ? QDateTime::fromMSecsSinceEpoch(s.lastSeenCompleteMs)
                                       .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                 : kDash);

  // Info
  totalSize_->setText(pfd::base::formatBytes(s.totalBytes));
  savePath_->setText(s.savePath.isEmpty() ? kDash : s.savePath);
  comment_->setText(s.comment.isEmpty() ? kDash : s.comment);
  addedOn_->setText(pfd::base::isValidMs(s.addedAtMs)
                        ? QDateTime::fromMSecsSinceEpoch(s.addedAtMs)
                              .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                        : kDash);
  infoHashV1_->setText(s.infoHashV1.isEmpty() ? kDash : s.infoHashV1);
  infoHashV2_->setText(s.infoHashV2.isEmpty() ? kDash : s.infoHashV2);

  if (s.pieces > 0 && s.pieceLength > 0) {
    pieces_->setText(
        QStringLiteral("%1 x %2").arg(s.pieces).arg(pfd::base::formatBytes(s.pieceLength)));
  } else {
    pieces_->setText(kDash);
  }
  createdBy_->setText(s.createdBy.isEmpty() ? kDash : s.createdBy);
  completedOn_->setText(pfd::base::isValidMs(s.completedOnMs)
                            ? QDateTime::fromMSecsSinceEpoch(s.completedOnMs)
                                  .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                            : kDash);
  createdOn_->setText(pfd::base::isValidMs(s.createdOnMs)
                          ? QDateTime::fromMSecsSinceEpoch(s.createdOnMs)
                                .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                          : kDash);
  isPrivate_->setText(s.isPrivate < 0   ? kDash
                      : s.isPrivate > 0 ? QStringLiteral("是")
                                        : QStringLiteral("否"));
}

void GeneralDetailPage::clear() {
  progressBar_->setValue(0);
  progressLabel_->setText(QStringLiteral("0.0%"));
  for (auto* l : {activeTime_,  eta_,       connections_, downloaded_,   uploaded_,
                  seeds_,       dlSpeed_,   ulSpeed_,     peers_,        dlLimit_,
                  ulLimit_,     wasted_,    shareRatio_,  nextAnnounce_, lastSeenComplete_,
                  popularity_,  totalSize_, pieces_,      createdBy_,    addedOn_,
                  completedOn_, createdOn_, isPrivate_,   infoHashV1_,   infoHashV2_,
                  savePath_,    comment_}) {
    l->setText(kDash);
  }
}

}  // namespace pfd::ui
