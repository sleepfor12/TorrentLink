#include "ui/rss/rss_rules_page.h"

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
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "core/rss/rss_service.h"

namespace pfd::ui::rss {

RssRulesPage::RssRulesPage(QWidget* parent) : QWidget(parent) {
  buildLayout();
  bindSignals();
}

void RssRulesPage::setService(pfd::core::rss::RssService* service) {
  service_ = service;
  refreshTable();
  updateSwitchStatus();
}

void RssRulesPage::refreshTable() {
  ruleTable_->setRowCount(0);
  if (!service_) return;

  const auto& rules = service_->rules();
  ruleTable_->setRowCount(static_cast<int>(rules.size()));
  for (int i = 0; i < static_cast<int>(rules.size()); ++i) {
    const auto& r = rules[static_cast<std::size_t>(i)];

    auto* nameItem = new QTableWidgetItem(r.name);
    nameItem->setData(Qt::UserRole, r.id);
    ruleTable_->setItem(i, 0, nameItem);
    ruleTable_->setItem(i, 1, new QTableWidgetItem(
        r.enabled ? QStringLiteral("已启用") : QStringLiteral("已禁用")));

    QString scope = r.feed_ids.isEmpty() ? QStringLiteral("全部订阅源")
                                         : QStringLiteral("%1 个源").arg(r.feed_ids.size());
    ruleTable_->setItem(i, 2, new QTableWidgetItem(scope));
    ruleTable_->setItem(i, 3, new QTableWidgetItem(r.include_keywords.join(QStringLiteral(", "))));
    ruleTable_->setItem(i, 4, new QTableWidgetItem(r.exclude_keywords.join(QStringLiteral(", "))));
    ruleTable_->setItem(i, 5, new QTableWidgetItem(
        r.use_regex ? QStringLiteral("正则") : QStringLiteral("关键词")));
    ruleTable_->setItem(i, 6, new QTableWidgetItem(r.save_path.isEmpty()
                                                       ? QStringLiteral("默认")
                                                       : r.save_path));
  }
  updateSwitchStatus();
}

void RssRulesPage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(10);

  // ─── 三层开关状态 ───
  globalSwitch_ = new QLabel(this);
  globalSwitch_->setStyleSheet(QStringLiteral(
      "QLabel{background:#f5f8ff;border:1px solid #d9e5ff;border-radius:8px;"
      "padding:8px 14px;color:#41516d;font-size:13px;}"));
  root->addWidget(globalSwitch_);

  // ─── 工具栏 ───
  auto* toolbar = new QHBoxLayout();
  toolbar->setSpacing(8);
  addBtn_ = new QPushButton(QStringLiteral("新建规则"), this);
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

  // ─── 主体 splitter ───
  auto* splitter = new QSplitter(Qt::Vertical, this);

  // 表格
  ruleTable_ = new QTableWidget(splitter);
  ruleTable_->setColumnCount(7);
  ruleTable_->setHorizontalHeaderLabels(
      {QStringLiteral("名称"), QStringLiteral("状态"), QStringLiteral("订阅源范围"),
       QStringLiteral("包含"), QStringLiteral("排除"), QStringLiteral("匹配模式"),
       QStringLiteral("保存路径")});
  ruleTable_->horizontalHeader()->setStretchLastSection(true);
  ruleTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
  ruleTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
  ruleTable_->setSelectionMode(QAbstractItemView::SingleSelection);
  ruleTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  // 编辑表单
  formContainer_ = new QGroupBox(QStringLiteral("规则编辑"), splitter);
  auto* form = new QFormLayout(formContainer_);
  form->setContentsMargins(12, 12, 12, 12);
  form->setSpacing(8);

  nameEdit_ = new QLineEdit(formContainer_);
  nameEdit_->setPlaceholderText(QStringLiteral("规则名称"));
  form->addRow(QStringLiteral("名称："), nameEdit_);

  feedIdsEdit_ = new QLineEdit(formContainer_);
  feedIdsEdit_->setPlaceholderText(QStringLiteral("留空 = 全部订阅源；多个用逗号分隔（填订阅源 ID）"));
  form->addRow(QStringLiteral("订阅源："), feedIdsEdit_);

  includeEdit_ = new QLineEdit(formContainer_);
  includeEdit_->setPlaceholderText(QStringLiteral("多个用逗号分隔，例如：1080p, HEVC"));
  form->addRow(QStringLiteral("包含词："), includeEdit_);

  excludeEdit_ = new QLineEdit(formContainer_);
  excludeEdit_->setPlaceholderText(QStringLiteral("多个用逗号分隔，例如：x265, REMUX"));
  form->addRow(QStringLiteral("排除词："), excludeEdit_);

  regexCheck_ = new QCheckBox(QStringLiteral("将关键词作为正则表达式"), formContainer_);
  form->addRow(QStringLiteral("模式："), regexCheck_);

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

  categoryEdit_ = new QLineEdit(formContainer_);
  form->addRow(QStringLiteral("分类："), categoryEdit_);

  tagsEdit_ = new QLineEdit(formContainer_);
  tagsEdit_->setPlaceholderText(QStringLiteral("多个用逗号分隔"));
  form->addRow(QStringLiteral("标签："), tagsEdit_);

  saveFormBtn_ = new QPushButton(QStringLiteral("保存规则"), formContainer_);
  saveFormBtn_->setObjectName(QStringLiteral("PrimaryButton"));
  form->addRow(QString(), saveFormBtn_);

  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  root->addWidget(splitter, 1);
}

void RssRulesPage::bindSignals() {
  connect(addBtn_, &QPushButton::clicked, this, &RssRulesPage::onAddRule);
  connect(editBtn_, &QPushButton::clicked, this, &RssRulesPage::onEditSelected);
  connect(removeBtn_, &QPushButton::clicked, this, &RssRulesPage::onRemoveSelected);
  connect(toggleBtn_, &QPushButton::clicked, this, &RssRulesPage::onToggleEnabled);
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
      QMessageBox::warning(this, QStringLiteral("规则"), QStringLiteral("请填写规则名称。"));
      return;
    }

    pfd::core::rss::RssRule rule;
    rule.id = editingRuleId_;
    rule.name = name;
    const QString feedIds = feedIdsEdit_->text().trimmed();
    if (!feedIds.isEmpty()) {
      rule.feed_ids = feedIds.split(QStringLiteral(","), Qt::SkipEmptyParts);
      for (auto& fid : rule.feed_ids) fid = fid.trimmed();
    }
    const QString inc = includeEdit_->text().trimmed();
    if (!inc.isEmpty()) {
      rule.include_keywords = inc.split(QStringLiteral(","), Qt::SkipEmptyParts);
      for (auto& k : rule.include_keywords) k = k.trimmed();
    }
    const QString exc = excludeEdit_->text().trimmed();
    if (!exc.isEmpty()) {
      rule.exclude_keywords = exc.split(QStringLiteral(","), Qt::SkipEmptyParts);
      for (auto& k : rule.exclude_keywords) k = k.trimmed();
    }
    rule.use_regex = regexCheck_->isChecked();
    rule.save_path = savePathEdit_->text().trimmed();
    rule.category = categoryEdit_->text().trimmed();
    rule.tags_csv = tagsEdit_->text().trimmed();

    if (!editingRuleId_.isEmpty()) {
      auto existing = std::find_if(service_->rules().begin(), service_->rules().end(),
                                   [&](const pfd::core::rss::RssRule& r) {
                                     return r.id == editingRuleId_;
                                   });
      if (existing != service_->rules().end()) {
        rule.enabled = existing->enabled;
      }
    }

    service_->upsertRule(rule);
    service_->saveState();
    editingRuleId_.clear();
    nameEdit_->clear();
    feedIdsEdit_->clear();
    includeEdit_->clear();
    excludeEdit_->clear();
    regexCheck_->setChecked(false);
    savePathEdit_->clear();
    categoryEdit_->clear();
    tagsEdit_->clear();
    refreshTable();
  });

  connect(ruleTable_, &QTableWidget::currentCellChanged, this,
          [this](int, int, int, int) { onSelectionChanged(); });
}

void RssRulesPage::onAddRule() {
  editingRuleId_.clear();
  nameEdit_->clear();
  feedIdsEdit_->clear();
  includeEdit_->clear();
  excludeEdit_->clear();
  regexCheck_->setChecked(false);
  savePathEdit_->clear();
  categoryEdit_->clear();
  tagsEdit_->clear();
  nameEdit_->setFocus();
}

void RssRulesPage::onEditSelected() {
  if (!service_) return;
  const int row = ruleTable_->currentRow();
  if (row < 0) return;

  const QString ruleId = ruleTable_->item(row, 0)->data(Qt::UserRole).toString();
  for (const auto& r : service_->rules()) {
    if (r.id != ruleId) continue;
    editingRuleId_ = r.id;
    nameEdit_->setText(r.name);
    feedIdsEdit_->setText(r.feed_ids.join(QStringLiteral(", ")));
    includeEdit_->setText(r.include_keywords.join(QStringLiteral(", ")));
    excludeEdit_->setText(r.exclude_keywords.join(QStringLiteral(", ")));
    regexCheck_->setChecked(r.use_regex);
    savePathEdit_->setText(r.save_path);
    categoryEdit_->setText(r.category);
    tagsEdit_->setText(r.tags_csv);
    nameEdit_->setFocus();
    return;
  }
}

void RssRulesPage::onRemoveSelected() {
  if (!service_) return;
  const int row = ruleTable_->currentRow();
  if (row < 0) return;
  const QString ruleId = ruleTable_->item(row, 0)->data(Qt::UserRole).toString();
  service_->removeRule(ruleId);
  service_->saveState();
  refreshTable();
}

void RssRulesPage::onToggleEnabled() {
  if (!service_) return;
  const int row = ruleTable_->currentRow();
  if (row < 0) return;
  const QString ruleId = ruleTable_->item(row, 0)->data(Qt::UserRole).toString();
  for (auto r : service_->rules()) {
    if (r.id != ruleId) continue;
    r.enabled = !r.enabled;
    service_->upsertRule(r);
    service_->saveState();
    refreshTable();
    return;
  }
}

void RssRulesPage::onSelectionChanged() {
  const int row = ruleTable_->currentRow();
  editBtn_->setEnabled(row >= 0);
  removeBtn_->setEnabled(row >= 0);
  toggleBtn_->setEnabled(row >= 0);
}

void RssRulesPage::updateSwitchStatus() {
  if (!service_) {
    globalSwitch_->setText(QStringLiteral("自动下载状态：未初始化"));
    return;
  }

  const bool global = service_->autoDownloadEnabled();
  int feedsEnabled = 0;
  for (const auto& f : service_->feeds()) {
    if (f.auto_download_enabled) ++feedsEnabled;
  }
  int rulesEnabled = 0;
  for (const auto& r : service_->rules()) {
    if (r.enabled) ++rulesEnabled;
  }

  QString text = QStringLiteral("自动下载链路：全局%1  ·  %2 个订阅源已开启  ·  %3 条规则已启用")
                     .arg(global ? QStringLiteral("【开】") : QStringLiteral("【关】"))
                     .arg(feedsEnabled)
                     .arg(rulesEnabled);

  if (!global) {
    text += QStringLiteral("  ⚠ 全局开关关闭，自动下载不会触发");
  } else if (feedsEnabled == 0) {
    text += QStringLiteral("  ⚠ 无订阅源开启自动下载");
  } else if (rulesEnabled == 0) {
    text += QStringLiteral("  ⚠ 无已启用规则");
  }

  globalSwitch_->setText(text);
}

}  // namespace pfd::ui::rss
