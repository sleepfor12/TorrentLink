#include "ui/rss/rss_settings_page.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

#include "core/rss/rss_service.h"

namespace pfd::ui::rss {

RssSettingsPage::RssSettingsPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void RssSettingsPage::setService(pfd::core::rss::RssService* service) {
  service_ = service;
  loadFromService();
}

void RssSettingsPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(16, 16, 16, 16);
  root->setSpacing(16);

  auto* hint = new QLabel(
      QStringLiteral("配置 RSS 模块的全局行为。修改后点击「保存」生效。"), this);
  hint->setStyleSheet(QStringLiteral("color:#6b7280;font-size:12px;"));
  hint->setWordWrap(true);
  root->addWidget(hint);

  // --- Auto-download group ---
  auto* dlGroup = new QGroupBox(QStringLiteral("自动下载"), this);
  auto* dlForm = new QFormLayout(dlGroup);
  dlForm->setContentsMargins(12, 12, 12, 12);
  dlForm->setSpacing(8);

  globalAutoDownloadCheck_ = new QCheckBox(QStringLiteral("启用全局自动下载（总开关）"), dlGroup);
  dlForm->addRow(globalAutoDownloadCheck_);

  maxAutoDownloadsSpin_ = new QSpinBox(dlGroup);
  maxAutoDownloadsSpin_->setRange(1, 100);
  maxAutoDownloadsSpin_->setSuffix(QStringLiteral(" 条"));
  dlForm->addRow(QStringLiteral("每次刷新最大自动下载数："), maxAutoDownloadsSpin_);

  root->addWidget(dlGroup);

  // --- Refresh group ---
  auto* refreshGroup = new QGroupBox(QStringLiteral("刷新"), this);
  auto* refreshForm = new QFormLayout(refreshGroup);
  refreshForm->setContentsMargins(12, 12, 12, 12);
  refreshForm->setSpacing(8);

  refreshIntervalSpin_ = new QSpinBox(refreshGroup);
  refreshIntervalSpin_->setRange(5, 1440);
  refreshIntervalSpin_->setSuffix(QStringLiteral(" 分钟"));
  refreshForm->addRow(QStringLiteral("自动刷新间隔："), refreshIntervalSpin_);

  root->addWidget(refreshGroup);

  // --- History group ---
  auto* histGroup = new QGroupBox(QStringLiteral("历史与清理"), this);
  auto* histForm = new QFormLayout(histGroup);
  histForm->setContentsMargins(12, 12, 12, 12);
  histForm->setSpacing(8);

  historyMaxItemsSpin_ = new QSpinBox(histGroup);
  historyMaxItemsSpin_->setRange(100, 100000);
  historyMaxItemsSpin_->setSuffix(QStringLiteral(" 条"));
  histForm->addRow(QStringLiteral("最大保留条目数："), historyMaxItemsSpin_);

  historyMaxAgeSpin_ = new QSpinBox(histGroup);
  historyMaxAgeSpin_->setRange(0, 3650);
  historyMaxAgeSpin_->setSpecialValueText(QStringLiteral("不限制"));
  historyMaxAgeSpin_->setSuffix(QStringLiteral(" 天"));
  histForm->addRow(QStringLiteral("最长保留天数："), historyMaxAgeSpin_);

  root->addWidget(histGroup);

  // --- Save path info ---
  auto* pathGroup = new QGroupBox(QStringLiteral("默认保存路径"), this);
  auto* pathForm = new QFormLayout(pathGroup);
  pathForm->setContentsMargins(12, 12, 12, 12);
  pathForm->setSpacing(8);

  auto* pathHint = new QLabel(
      QStringLiteral("RSS 规则和剧集订阅中保存路径留空时，将使用全局的『默认下载目录』。\n"
                     "可在主菜单 - 首选项 - 下载 中修改，或在规则/剧集表单中用『浏览』指定独立路径。"),
      pathGroup);
  pathHint->setWordWrap(true);
  pathHint->setStyleSheet(QStringLiteral("color:#6b7280;font-size:12px;"));
  pathForm->addRow(pathHint);

  root->addWidget(pathGroup);

  // --- Player group ---
  auto* playerGroup = new QGroupBox(QStringLiteral("外部播放器"), this);
  auto* playerForm = new QFormLayout(playerGroup);
  playerForm->setContentsMargins(12, 12, 12, 12);
  playerForm->setSpacing(8);

  playerCommandEdit_ = new QLineEdit(playerGroup);
  playerCommandEdit_->setPlaceholderText(QStringLiteral("留空使用系统默认播放器，例如：mpv, vlc"));
  playerForm->addRow(QStringLiteral("播放器命令："), playerCommandEdit_);

  root->addWidget(playerGroup);

  // --- Save button ---
  saveBtn_ = new QPushButton(QStringLiteral("保存设置"), this);
  saveBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  root->addWidget(saveBtn_);

  root->addStretch();

  connect(saveBtn_, &QPushButton::clicked, this, &RssSettingsPage::onSave);
}

void RssSettingsPage::loadFromService() {
  if (!service_) return;
  const auto s = service_->settings();
  globalAutoDownloadCheck_->setChecked(s.global_auto_download);
  refreshIntervalSpin_->setValue(s.refresh_interval_minutes);
  maxAutoDownloadsSpin_->setValue(s.max_auto_downloads_per_refresh);
  historyMaxItemsSpin_->setValue(s.history_max_items);
  historyMaxAgeSpin_->setValue(s.history_max_age_days);
  playerCommandEdit_->setText(s.external_player_command);
}

void RssSettingsPage::onSave() {
  if (!service_) return;

  pfd::core::rss::RssSettings s;
  s.global_auto_download = globalAutoDownloadCheck_->isChecked();
  s.refresh_interval_minutes = refreshIntervalSpin_->value();
  s.max_auto_downloads_per_refresh = maxAutoDownloadsSpin_->value();
  s.history_max_items = historyMaxItemsSpin_->value();
  s.history_max_age_days = historyMaxAgeSpin_->value();
  s.external_player_command = playerCommandEdit_->text().trimmed();

  service_->applySettings(s);
  service_->saveState();

  Q_EMIT settingsSaved();

  QMessageBox::information(this, QStringLiteral("RSS 设置"),
                           QStringLiteral("设置已保存并立即生效。"));
}

}  // namespace pfd::ui::rss
