#include <QtCore/QFileInfo>
#include <QtCore/QPoint>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include "ui/main_window.h"

namespace pfd::ui {

void MainWindow::showTaskContextMenu(const QPoint& pos) {
  if (taskTable_ == nullptr) {
    return;
  }
  const QModelIndex idx = taskTable_->indexAt(pos);
  if (!idx.isValid()) {
    return;
  }

  const bool clickedRowAlreadySelected =
      taskTable_->selectionModel() != nullptr &&
      taskTable_->selectionModel()->isRowSelected(idx.row(), QModelIndex());
  if (!clickedRowAlreadySelected) {
    taskTable_->selectRow(idx.row());
  }

  struct SelectedTask {
    pfd::base::TaskId id;
    pfd::core::TaskSnapshot snap;
  };
  std::vector<SelectedTask> selected;
  if (taskTable_->selectionModel() != nullptr) {
    const auto rows = taskTable_->selectionModel()->selectedRows();
    for (const auto& mi : rows) {
      const int r = mi.row();
      if (r >= 0 && r < static_cast<int>(displayedSnapshots_.size())) {
        const auto& s = displayedSnapshots_[static_cast<std::size_t>(r)];
        if (!s.taskId.isNull()) {
          selected.push_back({s.taskId, s});
        }
      }
    }
  }
  if (selected.empty()) {
    return;
  }

  const int clickedRow = idx.row();
  const auto& anchorSnap =
      (clickedRow >= 0 && clickedRow < static_cast<int>(displayedSnapshots_.size()))
          ? displayedSnapshots_[static_cast<std::size_t>(clickedRow)]
          : selected.front().snap;
  const pfd::base::TaskId anchorId = anchorSnap.taskId;
  const bool multi = selected.size() > 1;

  bool anyRunning = false;
  bool anyStopped = false;
  for (const auto& s : selected) {
    const auto st = s.snap.status;
    if (st == pfd::base::TaskStatus::kDownloading || st == pfd::base::TaskStatus::kSeeding ||
        st == pfd::base::TaskStatus::kChecking || st == pfd::base::TaskStatus::kQueued) {
      anyRunning = true;
    }
    if (st == pfd::base::TaskStatus::kPaused || st == pfd::base::TaskStatus::kFinished ||
        st == pfd::base::TaskStatus::kError) {
      anyStopped = true;
    }
  }

  const QString countSuffix = multi ? QStringLiteral("（%1 项）").arg(selected.size()) : QString();

  QMenu menu(this);
  QAction* startAction = menu.addAction(QStringLiteral("启动任务") + countSuffix);
  QAction* stopAction = menu.addAction(QStringLiteral("停止任务") + countSuffix);
  QAction* forceStartAction = menu.addAction(QStringLiteral("强制启动") + countSuffix);
  QAction* removeAction = menu.addAction(QStringLiteral("移除") + countSuffix);
  menu.addSeparator();
  QAction* moveAction = menu.addAction(QStringLiteral("移动位置") + countSuffix);
  QAction* renameAction = menu.addAction(QStringLiteral("重命名"));
  QAction* trackerAction = menu.addAction(QStringLiteral("编辑 Tracker"));
  QAction* connLimitAction = menu.addAction(QStringLiteral("连接数上限..."));
  QAction* autoManagedAction = menu.addAction(QStringLiteral("自动 Torrent 管理"));
  autoManagedAction->setCheckable(true);
  autoManagedAction->setChecked(true);

  QAction* torrentOptionsAction = menu.addAction(QStringLiteral("Torrent 选项..."));
  QAction* previewAction = menu.addAction(QStringLiteral("预览文件..."));
  QAction* sequentialAction = menu.addAction(QStringLiteral("按顺序下载"));
  sequentialAction->setCheckable(true);
  sequentialAction->setChecked(false);
  QAction* firstLastAction = menu.addAction(QStringLiteral("先下载首尾文件块"));
  firstLastAction->setCheckable(true);
  firstLastAction->setChecked(false);
  QAction* forceRecheckAction = menu.addAction(QStringLiteral("强制重新检查") + countSuffix);
  QAction* reannounceAction = menu.addAction(QStringLiteral("强制重新汇报") + countSuffix);
  QAction* openTargetAction = menu.addAction(QStringLiteral("打开目标文件夹"));

  QMenu* queueMenu = menu.addMenu(QStringLiteral("队列"));
  QAction* queueTopAction = queueMenu->addAction(QStringLiteral("移到队列顶部"));
  QAction* queueUpAction = queueMenu->addAction(QStringLiteral("上移一位"));
  QAction* queueDownAction = queueMenu->addAction(QStringLiteral("下移一位"));
  QAction* queueBottomAction = queueMenu->addAction(QStringLiteral("移到队列底部"));
  QMenu* copyMenu = menu.addMenu(QStringLiteral("复制"));
  QAction* copyNameAction = copyMenu->addAction(QStringLiteral("名称"));
  QAction* copyInfoHashV1Action = copyMenu->addAction(QStringLiteral("信息哈希值 v1"));
  QAction* copyInfoHashV2Action = copyMenu->addAction(QStringLiteral("信息哈希值 v2"));
  QAction* copyMagnetAction = copyMenu->addAction(QStringLiteral("磁力链接"));
  QAction* copyTorrentIdAction = copyMenu->addAction(QStringLiteral("Torrent ID"));
  QAction* copyCommentAction = copyMenu->addAction(QStringLiteral("注释"));

  QMenu* categoryMenu = menu.addMenu(QStringLiteral("分类"));
  QAction* categoryCreate = categoryMenu->addAction(QStringLiteral("新建分类"));
  QAction* categoryReset = categoryMenu->addAction(QStringLiteral("重置分类"));

  QMenu* tagMenu = menu.addMenu(QStringLiteral("标签"));
  QAction* tagCreate = tagMenu->addAction(QStringLiteral("新建标签"));
  QAction* tagReset = tagMenu->addAction(QStringLiteral("重置标签"));

  startAction->setEnabled(anyStopped && onResumeTask_ != nullptr);
  stopAction->setEnabled(anyRunning && onPauseTask_ != nullptr);
  forceStartAction->setEnabled(onForceStartTask_ != nullptr);
  renameAction->setEnabled(!multi);
  trackerAction->setEnabled(!multi);
  connLimitAction->setEnabled(!multi);
  autoManagedAction->setEnabled(!multi);
  torrentOptionsAction->setEnabled(!multi);
  previewAction->setEnabled(!multi);
  sequentialAction->setEnabled(!multi);
  firstLastAction->setEnabled(!multi);
  openTargetAction->setEnabled(!multi);

  QAction* chosen = menu.exec(taskTable_->viewport()->mapToGlobal(pos));
  if (chosen == nullptr) {
    return;
  }

  if (chosen == startAction) {
    if (onResumeTask_) {
      for (const auto& s : selected) {
        if (s.snap.status == pfd::base::TaskStatus::kPaused ||
            s.snap.status == pfd::base::TaskStatus::kFinished ||
            s.snap.status == pfd::base::TaskStatus::kError) {
          onResumeTask_(s.id);
        }
      }
      appendLog(QStringLiteral("已提交启动请求：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == stopAction) {
    if (onPauseTask_) {
      for (const auto& s : selected) {
        if (s.snap.status == pfd::base::TaskStatus::kDownloading ||
            s.snap.status == pfd::base::TaskStatus::kSeeding ||
            s.snap.status == pfd::base::TaskStatus::kChecking ||
            s.snap.status == pfd::base::TaskStatus::kQueued) {
          onPauseTask_(s.id);
        }
      }
      appendLog(QStringLiteral("已提交停止请求：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == forceStartAction) {
    if (onForceStartTask_) {
      for (const auto& s : selected) {
        onForceStartTask_(s.id);
      }
      appendLog(QStringLiteral("已请求强制启动：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == removeAction) {
    if (!onRemoveTask_) {
      return;
    }
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("确认移除任务"));
    dlg.setModal(true);
    auto* layout = new QVBoxLayout(&dlg);
    layout->addWidget(
        new QLabel(QStringLiteral("确定要移除选中的 %1 项任务吗？").arg(selected.size()), &dlg));
    auto* deleteFilesCheck = new QCheckBox(QStringLiteral("同时删除已下载的文件"), &dlg);
    layout->addWidget(deleteFilesCheck);
    auto* buttons = new QDialogButtonBox(&dlg);
    auto* cancelBtn = buttons->addButton(QStringLiteral("取消"), QDialogButtonBox::RejectRole);
    auto* deleteBtn = buttons->addButton(QStringLiteral("删除"), QDialogButtonBox::AcceptRole);
    deleteBtn->setDefault(true);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(deleteBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(buttons);
    if (dlg.exec() == QDialog::Accepted) {
      const bool deleteFiles = deleteFilesCheck->isChecked();
      for (const auto& s : selected) {
        onRemoveTask_(s.id, deleteFiles);
      }
      appendLog(QStringLiteral("已提交移除请求：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == moveAction) {
    const QString path = QFileDialog::getExistingDirectory(this, QStringLiteral("选择新的保存位置"),
                                                           selectedSavePath());
    if (!path.isEmpty() && onMoveTask_) {
      for (const auto& s : selected) {
        onMoveTask_(s.id, path);
      }
      appendLog(QStringLiteral("已提交移动位置请求：%1 项任务 -> %2").arg(selected.size()).arg(path));
    }
    return;
  }
  if (chosen == renameAction) {
    bool ok = false;
    const QString name =
        QInputDialog::getText(this, QStringLiteral("重命名任务"), QStringLiteral("请输入新名称"),
                              QLineEdit::Normal, QString(), &ok);
    if (ok && !name.trimmed().isEmpty() && onRenameTask_) {
      onRenameTask_(anchorId, name.trimmed());
      appendLog(QStringLiteral("已更新名称：%1 -> %2").arg(anchorSnap.name, name.trimmed()));
    }
    return;
  }
  if (chosen == trackerAction) {
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("编辑 Tracker"));
    dlg.setModal(true);
    auto* layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(QStringLiteral("请输入 Tracker（每行一个）"), &dlg));
    auto* trackerEdit = new QTextEdit(&dlg);
    QStringList trackerLines;
    if (onQueryTaskTrackers_) {
      trackerLines = onQueryTaskTrackers_(anchorId);
    }
    if (trackerLines.isEmpty() && onQueryDefaultTrackers_) {
      trackerLines = onQueryDefaultTrackers_();
    }
    trackerEdit->setPlainText(trackerLines.join('\n'));
    layout->addWidget(trackerEdit);
    auto* defaultCheck = new QCheckBox(QStringLiteral("设为新下载任务默认 Tracker"), &dlg);
    if (onQueryDefaultTrackerEnabled_) {
      defaultCheck->setChecked(onQueryDefaultTrackerEnabled_());
    }
    layout->addWidget(defaultCheck);
    auto* buttons = new QDialogButtonBox(&dlg);
    auto* cancelBtn = buttons->addButton(QStringLiteral("取消"), QDialogButtonBox::RejectRole);
    auto* saveBtn = buttons->addButton(QStringLiteral("保存"), QDialogButtonBox::AcceptRole);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(buttons);
    if (dlg.exec() == QDialog::Accepted) {
      const QStringList trackers = trackerEdit->toPlainText().split('\n', Qt::SkipEmptyParts);
      if (onEditTrackers_) {
        onEditTrackers_(anchorId, trackers);
      }
      if (onDefaultTrackersChanged_) {
        onDefaultTrackersChanged_(defaultCheck->isChecked(), trackers);
      }
      appendLog(QStringLiteral("已更新 Tracker：%1（%2 条）")
                    .arg(anchorSnap.name)
                    .arg(trackers.size()));
    }
    return;
  }
  if (chosen == connLimitAction) {
    bool ok = false;
    const int v = QInputDialog::getInt(this, QStringLiteral("连接数上限"),
                                       QStringLiteral("设置该任务的连接数上限（0 表示不限制）"), 0,
                                       0, 20000, 10, &ok);
    if (ok && onSetTaskConnectionsLimit_) {
      onSetTaskConnectionsLimit_(anchorId, v);
      appendLog(QStringLiteral("已更新连接数上限：%1 -> %2").arg(anchorSnap.name).arg(v));
    }
    return;
  }
  if (chosen == autoManagedAction) {
    if (onSetAutoManagedTask_) {
      onSetAutoManagedTask_(anchorId, autoManagedAction->isChecked());
      appendLog(
          QStringLiteral("任务自动管理已%1：%2")
              .arg(autoManagedAction->isChecked() ? QStringLiteral("启用") : QStringLiteral("关闭"),
                   anchorSnap.name));
    }
    return;
  }
  if (chosen == torrentOptionsAction) {
    bool ok = false;
    const int v = QInputDialog::getInt(this, QStringLiteral("Torrent 选项"),
                                       QStringLiteral("设置该任务连接数上限（0 表示不限制）"), 0, 0,
                                       20000, 10, &ok);
    if (ok && onSetTaskConnectionsLimit_) {
      onSetTaskConnectionsLimit_(anchorId, v);
      appendLog(QStringLiteral("已从 Torrent 选项更新连接数上限：%1 -> %2")
                    .arg(anchorSnap.name)
                    .arg(v));
    }
    return;
  }
  if (chosen == previewAction) {
    if (onPreviewFile_) {
      onPreviewFile_(anchorId);
    } else {
      const QString dir = QFileInfo(anchorSnap.savePath).absoluteFilePath();
      QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    }
    appendLog(QStringLiteral("预览文件：%1").arg(anchorSnap.name));
    return;
  }
  if (chosen == sequentialAction) {
    if (onSetSequentialDownload_) {
      onSetSequentialDownload_(anchorId, sequentialAction->isChecked());
      appendLog(
          QStringLiteral("任务按顺序下载已%1：%2")
              .arg(sequentialAction->isChecked() ? QStringLiteral("启用") : QStringLiteral("关闭"),
                   anchorSnap.name));
    }
    return;
  }
  if (chosen == firstLastAction) {
    if (onFirstLastPiecePriority_) {
      onFirstLastPiecePriority_(anchorId, firstLastAction->isChecked());
      appendLog(
          QStringLiteral("首尾块优先下载已%1：%2")
              .arg(firstLastAction->isChecked() ? QStringLiteral("启用") : QStringLiteral("关闭"),
                   anchorSnap.name));
    }
    return;
  }
  if (chosen == forceRecheckAction) {
    if (onForceRecheckTask_) {
      for (const auto& s : selected) {
        onForceRecheckTask_(s.id);
      }
      appendLog(QStringLiteral("已请求强制重新检查：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == reannounceAction) {
    if (onForceReannounceTask_) {
      for (const auto& s : selected) {
        onForceReannounceTask_(s.id);
      }
      appendLog(QStringLiteral("已请求强制重新汇报：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == openTargetAction) {
    const QString dir = QFileInfo(anchorSnap.savePath).absoluteFilePath();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    appendLog(QStringLiteral("已打开目标文件夹：%1").arg(dir));
    return;
  }

  const auto payload =
      onQueryCopyPayload_ != nullptr ? onQueryCopyPayload_(anchorId) : CopyPayload{};

  if (chosen == copyNameAction) {
    const QString text = payload.name.isEmpty() ? anchorSnap.name : payload.name;
    QApplication::clipboard()->setText(text);
    appendLog(QStringLiteral("已复制名称：%1").arg(text));
    return;
  }
  if (chosen == copyInfoHashV1Action) {
    const QString text = payload.infoHashV1.isEmpty() ? QStringLiteral("N/A") : payload.infoHashV1;
    QApplication::clipboard()->setText(text);
    appendLog(QStringLiteral("已复制信息哈希值 v1：%1").arg(text));
    return;
  }
  if (chosen == copyInfoHashV2Action) {
    const QString text = payload.infoHashV2.isEmpty() ? QStringLiteral("N/A") : payload.infoHashV2;
    QApplication::clipboard()->setText(text);
    appendLog(QStringLiteral("已复制信息哈希值 v2：%1").arg(text));
    return;
  }
  if (chosen == copyMagnetAction) {
    const QString text = payload.magnet.isEmpty() ? QStringLiteral("N/A") : payload.magnet;
    QApplication::clipboard()->setText(text);
    appendLog(QStringLiteral("已复制磁力链接：%1").arg(text.left(96)));
    return;
  }
  if (chosen == copyTorrentIdAction) {
    const QString text =
        payload.torrentId.isEmpty() ? anchorId.toString(QUuid::WithoutBraces) : payload.torrentId;
    QApplication::clipboard()->setText(text);
    appendLog(QStringLiteral("已复制 Torrent ID：%1").arg(text));
    return;
  }
  if (chosen == copyCommentAction) {
    const QString text = payload.comment.isEmpty() ? QStringLiteral("N/A") : payload.comment;
    QApplication::clipboard()->setText(text);
    appendLog(QStringLiteral("已复制注释：%1").arg(text.left(96)));
    return;
  }
  if (chosen == categoryCreate) {
    bool ok = false;
    const QString c =
        QInputDialog::getText(this, QStringLiteral("新建分类"), QStringLiteral("请输入分类名称"),
                              QLineEdit::Normal, QString(), &ok);
    if (ok && !c.trimmed().isEmpty() && onCategoryChanged_) {
      for (const auto& s : selected) {
        onCategoryChanged_(s.id, c.trimmed());
      }
      appendLog(QStringLiteral("已更新分类：%1 项任务 -> %2").arg(selected.size()).arg(c.trimmed()));
    }
    return;
  }
  if (chosen == categoryReset) {
    if (onCategoryChanged_) {
      for (const auto& s : selected) {
        onCategoryChanged_(s.id, QStringLiteral("Default"));
      }
      appendLog(QStringLiteral("已重置分类：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == tagCreate) {
    bool ok = false;
    const QString t =
        QInputDialog::getText(this, QStringLiteral("新建标签"), QStringLiteral("请输入标签名称"),
                              QLineEdit::Normal, QString(), &ok);
    if (ok && !t.trimmed().isEmpty() && onTagsChanged_) {
      for (const auto& s : selected) {
        onTagsChanged_(s.id, QStringList{t.trimmed()});
      }
      appendLog(QStringLiteral("已更新标签：%1 项任务 -> %2").arg(selected.size()).arg(t.trimmed()));
    }
    return;
  }
  if (chosen == tagReset) {
    if (onTagsChanged_) {
      for (const auto& s : selected) {
        onTagsChanged_(s.id, {});
      }
      appendLog(QStringLiteral("已重置标签：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == queueTopAction) {
    if (onQueuePositionTop_) {
      for (const auto& s : selected) {
        onQueuePositionTop_(s.id);
      }
      appendLog(QStringLiteral("已移到队列顶部：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == queueUpAction) {
    if (onQueuePositionUp_) {
      for (const auto& s : selected) {
        onQueuePositionUp_(s.id);
      }
      appendLog(QStringLiteral("已上移一位：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == queueDownAction) {
    if (onQueuePositionDown_) {
      for (const auto& s : selected) {
        onQueuePositionDown_(s.id);
      }
      appendLog(QStringLiteral("已下移一位：%1 项任务").arg(selected.size()));
    }
    return;
  }
  if (chosen == queueBottomAction) {
    if (onQueuePositionBottom_) {
      for (const auto& s : selected) {
        onQueuePositionBottom_(s.id);
      }
      appendLog(QStringLiteral("已移到队列底部：%1 项任务").arg(selected.size()));
    }
    return;
  }
}

}  // namespace pfd::ui
