#include "ui/rss/rss_series_page.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "core/rss/rss_service.h"

namespace pfd::ui::rss {

RssSeriesPage::RssSeriesPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
  bindSignals();
}

void RssSeriesPage::setService(pfd::core::rss::RssService* service) {
  service_ = service;
  refreshTable();
}

void RssSeriesPage::refreshTable() {
  seriesTable_->setRowCount(0);
  if (!service_) return;

  const auto& subs = service_->series();
  seriesTable_->setRowCount(static_cast<int>(subs.size()));
  for (int i = 0; i < static_cast<int>(subs.size()); ++i) {
    const auto& s = subs[static_cast<std::size_t>(i)];

    auto* nameItem = new QTableWidgetItem(s.name);
    nameItem->setData(Qt::UserRole, s.id);
    seriesTable_->setItem(i, 0, nameItem);
    seriesTable_->setItem(i, 1, new QTableWidgetItem(
        s.enabled ? QStringLiteral("启用") : QStringLiteral("暂停")));
    seriesTable_->setItem(i, 2, new QTableWidgetItem(
        s.auto_download_enabled ? QStringLiteral("是") : QStringLiteral("否")));
    seriesTable_->setItem(i, 3, new QTableWidgetItem(
        s.season >= 0 ? QStringLiteral("S%1").arg(s.season, 2, 10, QLatin1Char('0'))
                      : QStringLiteral("全部")));
    seriesTable_->setItem(i, 4, new QTableWidgetItem(
        s.last_episode_num > 0 ? QStringLiteral("E%1").arg(s.last_episode_num, 2, 10, QLatin1Char('0'))
                               : QStringLiteral("-")));
    seriesTable_->setItem(i, 5, new QTableWidgetItem(s.quality_keywords.join(QStringLiteral(", "))));
    seriesTable_->setItem(i, 6, new QTableWidgetItem(s.aliases.join(QStringLiteral(", "))));
  }
}

void RssSeriesPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(10);

  auto* hint = new QLabel(
      QStringLiteral("管理剧集订阅。系统自动从 RSS 条目中提取集数信息，匹配订阅后按规则自动下载新集。"),
      this);
  hint->setStyleSheet(QStringLiteral("color:#6b7280;font-size:12px;"));
  hint->setWordWrap(true);
  root->addWidget(hint);

  auto* toolbar = new QHBoxLayout();
  toolbar->setSpacing(8);
  addBtn_ = new QPushButton(QStringLiteral("新建订阅"), this);
  addBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  editBtn_ = new QPushButton(QStringLiteral("编辑"), this);
  removeBtn_ = new QPushButton(QStringLiteral("删除"), this);
  toggleBtn_ = new QPushButton(QStringLiteral("启用/禁用"), this);
  toolbar->addWidget(addBtn_);
  toolbar->addWidget(editBtn_);
  toolbar->addWidget(toggleBtn_);
  toolbar->addWidget(removeBtn_);
  toolbar->addStretch();
  root->addLayout(toolbar);

  auto* splitter = new QSplitter(Qt::Vertical, this);

  seriesTable_ = new QTableWidget(splitter);
  seriesTable_->setColumnCount(7);
  seriesTable_->setHorizontalHeaderLabels(
      {QStringLiteral("名称"), QStringLiteral("状态"), QStringLiteral("自动下载"),
       QStringLiteral("季"), QStringLiteral("最新集"), QStringLiteral("画质偏好"),
       QStringLiteral("别名")});
  seriesTable_->horizontalHeader()->setStretchLastSection(true);
  seriesTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  seriesTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  seriesTable_->setSelectionMode(QAbstractItemView::SingleSelection);
  seriesTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  formContainer_ = new QGroupBox(QStringLiteral("剧集订阅编辑"), splitter);
  auto* form = new QFormLayout(formContainer_);
  form->setContentsMargins(12, 12, 12, 12);
  form->setSpacing(8);

  nameEdit_ = new QLineEdit(formContainer_);
  nameEdit_->setPlaceholderText(QStringLiteral("作品名称（用于匹配标题）"));
  form->addRow(QStringLiteral("名称："), nameEdit_);

  aliasesEdit_ = new QLineEdit(formContainer_);
  aliasesEdit_->setPlaceholderText(QStringLiteral("逗号分隔，例如：进击的巨人, Attack on Titan, Shingeki"));
  form->addRow(QStringLiteral("别名："), aliasesEdit_);

  qualityEdit_ = new QLineEdit(formContainer_);
  qualityEdit_->setPlaceholderText(QStringLiteral("逗号分隔，例如：1080p, HEVC"));
  form->addRow(QStringLiteral("画质偏好："), qualityEdit_);

  seasonSpin_ = new QSpinBox(formContainer_);
  seasonSpin_->setRange(-1, 99);
  seasonSpin_->setSpecialValueText(QStringLiteral("全部季"));
  seasonSpin_->setValue(-1);
  form->addRow(QStringLiteral("季："), seasonSpin_);

  lastEpSpin_ = new QSpinBox(formContainer_);
  lastEpSpin_->setRange(0, 9999);
  lastEpSpin_->setSpecialValueText(QStringLiteral("从头开始"));
  form->addRow(QStringLiteral("已看到集："), lastEpSpin_);

  autoDownloadCheck_ = new QCheckBox(QStringLiteral("开启自动下载"), formContainer_);
  form->addRow(QStringLiteral("自动下载："), autoDownloadCheck_);

  auto* savePathRow = new QWidget(formContainer_);
  auto* savePathLayout = new QHBoxLayout(savePathRow);
  savePathLayout->setContentsMargins(0, 0, 0, 0);
  savePathLayout->setSpacing(4);
  savePathEdit_ = new QLineEdit(savePathRow);
  savePathEdit_->setPlaceholderText(QStringLiteral("留空则使用「设置 → 默认下载目录」"));
  browseSavePathBtn_ = new QPushButton(QStringLiteral("浏览"), savePathRow);
  browseSavePathBtn_->setFixedWidth(60);
  savePathLayout->addWidget(savePathEdit_, 1);
  savePathLayout->addWidget(browseSavePathBtn_);
  form->addRow(QStringLiteral("保存路径："), savePathRow);

  saveFormBtn_ = new QPushButton(QStringLiteral("保存"), formContainer_);
  saveFormBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  form->addRow(QString(), saveFormBtn_);

  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  root->addWidget(splitter, 1);
}

void RssSeriesPage::bindSignals() {
  connect(addBtn_, &QPushButton::clicked, this, &RssSeriesPage::onAddSeries);
  connect(editBtn_, &QPushButton::clicked, this, &RssSeriesPage::onEditSelected);
  connect(removeBtn_, &QPushButton::clicked, this, &RssSeriesPage::onRemoveSelected);
  connect(toggleBtn_, &QPushButton::clicked, this, &RssSeriesPage::onToggleEnabled);

  connect(browseSavePathBtn_, &QPushButton::clicked, this, [this]() {
    const QString cur = savePathEdit_->text().trimmed();
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择保存路径"), cur);
    if (!dir.isEmpty()) savePathEdit_->setText(dir);
  });

  connect(saveFormBtn_, &QPushButton::clicked, this, [this]() {
    if (!service_) return;
    const QString name = nameEdit_->text().trimmed();
    if (name.isEmpty()) {
      QMessageBox::warning(this, QStringLiteral("剧集订阅"), QStringLiteral("请填写名称。"));
      return;
    }

    pfd::core::rss::SeriesSubscription sub;
    sub.id = editingSeriesId_;
    sub.name = name;
    const QString aliases = aliasesEdit_->text().trimmed();
    if (!aliases.isEmpty()) {
      sub.aliases = aliases.split(QStringLiteral(","), Qt::SkipEmptyParts);
      for (auto& a : sub.aliases) a = a.trimmed();
    }
    const QString quality = qualityEdit_->text().trimmed();
    if (!quality.isEmpty()) {
      sub.quality_keywords = quality.split(QStringLiteral(","), Qt::SkipEmptyParts);
      for (auto& q : sub.quality_keywords) q = q.trimmed();
    }
    sub.season = seasonSpin_->value();
    sub.last_episode_num = lastEpSpin_->value();
    sub.auto_download_enabled = autoDownloadCheck_->isChecked();
    sub.save_path = savePathEdit_->text().trimmed();

    if (!editingSeriesId_.isEmpty()) {
      for (const auto& existing : service_->series()) {
        if (existing.id == editingSeriesId_) {
          sub.enabled = existing.enabled;
          break;
        }
      }
    }

    service_->upsertSeries(sub);
    service_->saveState();
    editingSeriesId_.clear();
    nameEdit_->clear();
    aliasesEdit_->clear();
    qualityEdit_->clear();
    seasonSpin_->setValue(-1);
    lastEpSpin_->setValue(0);
    autoDownloadCheck_->setChecked(false);
    savePathEdit_->clear();
    refreshTable();
  });
}

void RssSeriesPage::onAddSeries() {
  editingSeriesId_.clear();
  nameEdit_->clear();
  aliasesEdit_->clear();
  qualityEdit_->clear();
  seasonSpin_->setValue(-1);
  lastEpSpin_->setValue(0);
  autoDownloadCheck_->setChecked(false);
  savePathEdit_->clear();
  nameEdit_->setFocus();
}

void RssSeriesPage::onEditSelected() {
  if (!service_) return;
  const int row = seriesTable_->currentRow();
  if (row < 0) return;
  const QString sid = seriesTable_->item(row, 0)->data(Qt::UserRole).toString();
  for (const auto& s : service_->series()) {
    if (s.id != sid) continue;
    editingSeriesId_ = s.id;
    nameEdit_->setText(s.name);
    aliasesEdit_->setText(s.aliases.join(QStringLiteral(", ")));
    qualityEdit_->setText(s.quality_keywords.join(QStringLiteral(", ")));
    seasonSpin_->setValue(s.season);
    lastEpSpin_->setValue(s.last_episode_num);
    autoDownloadCheck_->setChecked(s.auto_download_enabled);
    savePathEdit_->setText(s.save_path);
    nameEdit_->setFocus();
    return;
  }
}

void RssSeriesPage::onRemoveSelected() {
  if (!service_) return;
  const int row = seriesTable_->currentRow();
  if (row < 0) return;
  const QString sid = seriesTable_->item(row, 0)->data(Qt::UserRole).toString();
  service_->removeSeries(sid);
  service_->saveState();
  refreshTable();
}

void RssSeriesPage::onToggleEnabled() {
  if (!service_) return;
  const int row = seriesTable_->currentRow();
  if (row < 0) return;
  const QString sid = seriesTable_->item(row, 0)->data(Qt::UserRole).toString();
  for (auto s : service_->series()) {
    if (s.id != sid) continue;
    s.enabled = !s.enabled;
    service_->upsertSeries(s);
    service_->saveState();
    refreshTable();
    return;
  }
}

}  // namespace pfd::ui::rss
