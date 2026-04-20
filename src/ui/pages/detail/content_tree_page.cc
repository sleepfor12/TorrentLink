#include "ui/pages/detail/content_tree_page.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtWidgets/QTreeWidgetItemIterator>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>

#include "base/format.h"
#include "core/external_player.h"
#include "core/logger.h"
#include "ui/input_ime_utils.h"

namespace pfd::ui {

namespace {
enum Role {
  kPathRole = Qt::UserRole + 1,
  kFileIndexRole,
  kIsDirRole,
  kPriorityRole,
};

QString priorityText(pfd::core::TaskFilePriorityLevel p) {
  using P = pfd::core::TaskFilePriorityLevel;
  switch (p) {
    case P::kNotDownloaded:
      return QStringLiteral("未下载");
    case P::kDoNotDownload:
      return QStringLiteral("不下载");
    case P::kHigh:
      return QStringLiteral("高");
    case P::kHighest:
      return QStringLiteral("最高");
    case P::kFileOrder:
      return QStringLiteral("按文件顺序");
    case P::kNormal:
    default:
      return QStringLiteral("正常");
  }
}
}  // namespace

ContentTreePage::ContentTreePage(QWidget* parent) : QWidget(parent) {
  buildLayout();
}

void ContentTreePage::setHandlers(QueryFilesFn queryFn, SetPriorityFn priorityFn,
                                  RenameFn renameFn) {
  queryFilesFn_ = std::move(queryFn);
  setPriorityFn_ = std::move(priorityFn);
  renameFn_ = std::move(renameFn);
}

void ContentTreePage::setSnapshot(const pfd::core::TaskSnapshot& snap) {
  taskId_ = snap.taskId;
  savePath_ = snap.savePath;
  LOG_DEBUG(QStringLiteral("[ui.content] setSnapshot taskId=%1 savePath=%2")
                .arg(taskId_.toString(), savePath_));
  reloadTree();
}

void ContentTreePage::clear() {
  taskId_ = {};
  savePath_.clear();
  if (tree_ != nullptr)
    tree_->clear();
}

void ContentTreePage::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(8);

  auto* top = new QHBoxLayout();
  top->addStretch(1);
  searchEdit_ = new QLineEdit(this);
  searchEdit_->setPlaceholderText(QStringLiteral("搜索内容名称"));
  searchEdit_->setClearButtonEnabled(true);
  selectAllBtn_ = new QPushButton(QStringLiteral("全选"), this);
  selectNoneBtn_ = new QPushButton(QStringLiteral("全不选"), this);
  top->addWidget(searchEdit_);
  top->addWidget(selectAllBtn_);
  top->addWidget(selectNoneBtn_);
  root->addLayout(top);

  tree_ = new QTreeWidget(this);
  tree_->setColumnCount(5);
  tree_->setHeaderLabels({QStringLiteral("名称"), QStringLiteral("大小"), QStringLiteral("进度"),
                          QStringLiteral("优先级"), QStringLiteral("剩余可用性")});
  tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tree_->setContextMenuPolicy(Qt::CustomContextMenu);
  tree_->header()->setStretchLastSection(false);
  tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  root->addWidget(tree_, 1);

  connectImeAwareLineEditApply(this, searchEdit_, 120, [this]() { applySearchFilter(); });
  connect(selectAllBtn_, &QPushButton::clicked, this, [this]() {
    if (tree_ == nullptr)
      return;
    tree_->clearSelection();
    for (QTreeWidgetItemIterator it(tree_); *it != nullptr; ++it) {
      QTreeWidgetItem* item = *it;
      if (item->isHidden())
        continue;
      if (!item->data(0, kIsDirRole).toBool())
        item->setSelected(true);
    }
  });
  connect(selectNoneBtn_, &QPushButton::clicked, this, [this]() {
    if (tree_ != nullptr)
      tree_->clearSelection();
  });
  connect(tree_, &QTreeWidget::customContextMenuRequested, this,
          [this](const QPoint& pos) { showContextMenu(pos); });
}

void ContentTreePage::reloadTree() {
  if (tree_ == nullptr)
    return;
  tree_->clear();
  if (taskId_.isNull() || !queryFilesFn_)
    return;

  const auto files = queryFilesFn_(taskId_);
  LOG_DEBUG(QStringLiteral("[ui.content] reload taskId=%1 file_count=%2")
                .arg(taskId_.toString())
                .arg(files.size()));
  QHash<QString, QTreeWidgetItem*> nodes;
  for (const auto& f : files) {
    const QStringList parts = f.logicalPath.split('/', Qt::SkipEmptyParts);
    QString acc;
    QTreeWidgetItem* parent = nullptr;
    for (int i = 0; i < parts.size(); ++i) {
      acc = acc.isEmpty() ? parts[i] : (acc + QStringLiteral("/") + parts[i]);
      QTreeWidgetItem* node = nodes.value(acc, nullptr);
      if (node == nullptr) {
        node = new QTreeWidgetItem();
        node->setText(0, parts[i]);
        node->setData(0, kPathRole, acc);
        const bool isLeaf = (i == parts.size() - 1);
        node->setData(0, kIsDirRole, !isLeaf);
        node->setData(0, kFileIndexRole, isLeaf ? f.fileIndex : -1);
        if (parent != nullptr)
          parent->addChild(node);
        else
          tree_->addTopLevelItem(node);
        nodes.insert(acc, node);
      }
      parent = node;
    }
    if (parent != nullptr) {
      parent->setText(1, pfd::base::formatBytes(f.sizeBytes));
      parent->setText(2, QStringLiteral("%1%").arg(f.progress01 * 100.0, 0, 'f', 1));
      parent->setData(0, kPriorityRole, static_cast<int>(f.priority));
      parent->setText(3, priorityText(f.priority));
      parent->setText(4, f.availability >= 0.0 ? QString::number(f.availability, 'f', 2)
                                               : QStringLiteral("--"));
      parent->setData(1, Qt::UserRole, f.sizeBytes);
      parent->setData(2, Qt::UserRole, f.downloadedBytes);
      parent->setData(4, Qt::UserRole, f.availability);
    }
  }

  for (QTreeWidgetItemIterator it(tree_); *it != nullptr; ++it) {
    QTreeWidgetItem* item = *it;
    if (!item->data(0, kIsDirRole).toBool())
      continue;
    qint64 size = 0;
    qint64 downloaded = 0;
    double minAvail = -1.0;
    QSet<int> ps;
    for (int i = 0; i < item->childCount(); ++i) {
      QTreeWidgetItem* c = item->child(i);
      size += c->data(1, Qt::UserRole).toLongLong();
      downloaded += c->data(2, Qt::UserRole).toLongLong();
      const double av = c->data(4, Qt::UserRole).toDouble();
      if (av >= 0.0 && (minAvail < 0.0 || av < minAvail))
        minAvail = av;
      ps.insert(c->data(0, kPriorityRole).toInt());
    }
    item->setData(1, Qt::UserRole, size);
    item->setData(2, Qt::UserRole, downloaded);
    item->setData(4, Qt::UserRole, minAvail);
    item->setText(1, pfd::base::formatBytes(size));
    const double p =
        size > 0 ? std::clamp(static_cast<double>(downloaded) / static_cast<double>(size), 0.0, 1.0)
                 : 0.0;
    item->setText(2, QStringLiteral("%1%").arg(p * 100.0, 0, 'f', 1));
    item->setText(4, minAvail >= 0.0 ? QString::number(minAvail, 'f', 2) : QStringLiteral("--"));
    if (ps.size() == 1) {
      item->setData(0, kPriorityRole, *ps.begin());
      item->setText(
          3, priorityText(static_cast<pfd::core::TaskFilePriorityLevel>(*ps.begin())));
    } else {
      item->setData(0, kPriorityRole, -1);
      item->setText(3, QStringLiteral("混合"));
    }
  }
  tree_->expandToDepth(1);
  applySearchFilter();
}

void ContentTreePage::applySearchFilter() {
  if (tree_ == nullptr)
    return;
  const QString key = searchEdit_ != nullptr ? searchEdit_->text().trimmed().toLower() : QString();
  for (QTreeWidgetItemIterator it(tree_); *it != nullptr; ++it) {
    (*it)->setHidden(false);
  }
  if (key.isEmpty())
    return;
  int visibleCount = 0;
  for (QTreeWidgetItemIterator it(tree_); *it != nullptr; ++it) {
    QTreeWidgetItem* item = *it;
    bool visible = item->text(0).toLower().contains(key);
    for (int i = 0; !visible && i < item->childCount(); ++i) {
      visible = !item->child(i)->isHidden();
    }
    item->setHidden(!visible);
    if (visible)
      ++visibleCount;
    if (visible) {
      QTreeWidgetItem* p = item->parent();
      while (p != nullptr) {
        p->setHidden(false);
        p = p->parent();
      }
    }
  }
  LOG_DEBUG(
      QStringLiteral("[ui.content] search key=%1 visible_items=%2").arg(key).arg(visibleCount));
}

void ContentTreePage::showContextMenu(const QPoint& pos) {
  if (tree_ == nullptr || taskId_.isNull())
    return;
  QTreeWidgetItem* item = tree_->itemAt(pos);
  if (item == nullptr)
    return;
  auto items = tree_->selectedItems();
  if (items.isEmpty())
    items.push_back(item);

  QMenu menu(this);
  const bool isDir = item->data(0, kIsDirRole).toBool();
  const QString logical = logicalPathForItem(item);
  const bool hasOpenPath = !savePath_.trimmed().isEmpty() && !logical.trimmed().isEmpty();
  auto* openFolderAct =
      menu.addAction(isDir ? QStringLiteral("打开文件夹") : QStringLiteral("打开文件所在文件夹"));
  auto* openContainingAct = menu.addAction(QStringLiteral("打开包含文件夹"));
  auto* renameAct = menu.addAction(QStringLiteral("重命名"));
  QMenu* priorityMenu = menu.addMenu(QStringLiteral("设置优先级"));
  auto addPriority = [priorityMenu](const QString& text, pfd::core::TaskFilePriorityLevel lv) {
    QAction* a = priorityMenu->addAction(text);
    a->setData(static_cast<int>(lv));
  };
  addPriority(QStringLiteral("未下载"), pfd::core::TaskFilePriorityLevel::kNotDownloaded);
  addPriority(QStringLiteral("不下载"), pfd::core::TaskFilePriorityLevel::kDoNotDownload);
  addPriority(QStringLiteral("正常"), pfd::core::TaskFilePriorityLevel::kNormal);
  addPriority(QStringLiteral("高"), pfd::core::TaskFilePriorityLevel::kHigh);
  addPriority(QStringLiteral("最高"), pfd::core::TaskFilePriorityLevel::kHighest);
  addPriority(QStringLiteral("按文件顺序"), pfd::core::TaskFilePriorityLevel::kFileOrder);

  openFolderAct->setEnabled(hasOpenPath);
  openContainingAct->setEnabled(hasOpenPath);
  renameAct->setEnabled(items.size() == 1);

  QAction* chosen = menu.exec(tree_->viewport()->mapToGlobal(pos));
  if (chosen == nullptr)
    return;
  if (chosen == openFolderAct || chosen == openContainingAct) {
    const QString full = QDir(savePath_).filePath(logical);
    QString openPath = full;
    if (chosen == openContainingAct || !isDir) {
      openPath = QFileInfo(full).absolutePath();
    }
    if (pfd::core::ExternalPlayer::openFolder(openPath) != pfd::core::ExternalPlayer::Result::Ok) {
      LOG_WARN(QStringLiteral("[ui.content] open path failed taskId=%1 path=%2")
                   .arg(taskId_.toString(), openPath));
      QMessageBox::warning(this, QStringLiteral("提示"),
                           QStringLiteral("目标路径不存在：%1").arg(openPath));
    } else {
      LOG_INFO(QStringLiteral("[ui.content] open path taskId=%1 path=%2")
                   .arg(taskId_.toString(), openPath));
    }
    return;
  }
  if (chosen == renameAct) {
    const QString logical = logicalPathForItem(item);
    const QString oldName = QFileInfo(logical).fileName();
    bool ok = false;
    const QString newName = QInputDialog::getText(
        this, QStringLiteral("重命名"), QStringLiteral("新名称"), QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.trimmed().isEmpty() && renameFn_) {
      LOG_INFO(QStringLiteral("[ui.content] rename request taskId=%1 path=%2 new=%3")
                   .arg(taskId_.toString(), logical, newName.trimmed()));
      renameFn_(taskId_, logical, newName.trimmed());
      applyRenameLocally(item, newName.trimmed());
      applySearchFilter();
    }
    return;
  }
  if (chosen->parentWidget() == priorityMenu) {
    applyPriorityToSelection(
        static_cast<pfd::core::TaskFilePriorityLevel>(chosen->data().toInt()));
  }
}

void ContentTreePage::applyPriorityToSelection(pfd::core::TaskFilePriorityLevel level) {
  if (!setPriorityFn_ || tree_ == nullptr)
    return;
  std::vector<int> all;
  for (QTreeWidgetItem* item : tree_->selectedItems()) {
    auto v = collectFileIndices(item);
    all.insert(all.end(), v.begin(), v.end());
  }
  std::sort(all.begin(), all.end());
  all.erase(std::unique(all.begin(), all.end()), all.end());
  if (all.empty())
    return;
  LOG_INFO(QStringLiteral("[ui.content] set priority taskId=%1 level=%2 files=%3")
               .arg(taskId_.toString())
               .arg(static_cast<int>(level))
               .arg(all.size()));
  setPriorityFn_(taskId_, all, level);
  reloadTree();
}

std::vector<int> ContentTreePage::collectFileIndices(QTreeWidgetItem* item) const {
  std::vector<int> out;
  if (item == nullptr)
    return out;
  const int idx = item->data(0, kFileIndexRole).toInt();
  if (idx >= 0) {
    out.push_back(idx);
    return out;
  }
  for (int i = 0; i < item->childCount(); ++i) {
    auto child = collectFileIndices(item->child(i));
    out.insert(out.end(), child.begin(), child.end());
  }
  return out;
}

QString ContentTreePage::logicalPathForItem(QTreeWidgetItem* item) const {
  if (item == nullptr)
    return {};
  return item->data(0, kPathRole).toString();
}

void ContentTreePage::applyRenameLocally(QTreeWidgetItem* item, const QString& newName) {
  if (item == nullptr)
    return;
  const QString oldPath = logicalPathForItem(item);
  if (oldPath.isEmpty())
    return;
  const int slash = oldPath.lastIndexOf('/');
  const QString parent = slash >= 0 ? oldPath.left(slash) : QString();
  const QString newPath = parent.isEmpty() ? newName : (parent + QStringLiteral("/") + newName);
  item->setText(0, newName);
  rewritePathRecursive(item, oldPath, newPath);
}

void ContentTreePage::rewritePathRecursive(QTreeWidgetItem* item, const QString& oldPrefix,
                                           const QString& newPrefix) {
  if (item == nullptr)
    return;
  const QString curr = logicalPathForItem(item);
  if (curr == oldPrefix || curr.startsWith(oldPrefix + QStringLiteral("/"))) {
    QString next = curr;
    next.replace(0, oldPrefix.size(), newPrefix);
    item->setData(0, kPathRole, next);
  }
  for (int i = 0; i < item->childCount(); ++i) {
    rewritePathRecursive(item->child(i), oldPrefix, newPrefix);
  }
}

}  // namespace pfd::ui
