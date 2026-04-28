#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QMetaObject>
#include <QtCore/QPoint>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QVector>
#include <QtGui/QBrush>
#include <QtGui/QClipboard>
#include <QtGui/QColor>
#include <QtGui/QDesktopServices>
#include <QtGui/QMouseEvent>
#include <QtGui/QShortcut>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTableWidgetItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <thread>

#include "base/types.h"
#include "core/config_service.h"
#include "core/logger.h"
#include "core/torrent_creator.h"
#include "ui/main_window.h"

namespace pfd::ui {

void MainWindow::bindSignals() {
  if (openTorrentFileAction_ != nullptr) {
    connect(openTorrentFileAction_, &QAction::triggered, this,
            [this]() { openTorrentFileFromMenu(); });
  }
  if (openTorrentLinksAction_ != nullptr) {
    connect(openTorrentLinksAction_, &QAction::triggered, this,
            [this]() { openTorrentLinksFromMenu(); });
  }
  if (exitAction_ != nullptr) {
    connect(exitAction_, &QAction::triggered, this, [this]() {
      forceQuit_ = true;
      close();
    });
  }
  if (preferencesAction_ != nullptr) {
    connect(preferencesAction_, &QAction::triggered, this, [this]() { openPreferences(); });
  }
  if (logCenterAction_ != nullptr) {
    connect(logCenterAction_, &QAction::triggered, this, [this]() { openLogCenter(); });
  }
  if (createTorrentAction_ != nullptr) {
    connect(createTorrentAction_, &QAction::triggered, this,
            [this]() { openCreateTorrentDialog(); });
  }
  if (manageCookiesAction_ != nullptr) {
    connect(manageCookiesAction_, &QAction::triggered, this,
            [this]() { openCookieManagerDialog(); });
  }
  if (showLogAction_ != nullptr) {
    connect(showLogAction_, &QAction::triggered, this, [this]() { openLogCenter(); });
  }
  if (refreshListAction_ != nullptr) {
    connect(refreshListAction_, &QAction::triggered, this, [this]() { refreshTasks(snapshots_); });
  }
  if (themeAction_ != nullptr) {
    connect(themeAction_, &QAction::triggered, this, [this]() {
      QMessageBox::information(this, QStringLiteral("界面主题"),
                               QStringLiteral("界面主题切换功能开发中。"));
    });
  }
  if (helpAction_ != nullptr) {
    connect(helpAction_, &QAction::triggered, this, [this]() {
      QMessageBox::information(
          this, QStringLiteral("使用说明"),
          QStringLiteral("<h3>P2P 文件下载系统 — 使用说明</h3>"
                         "<ul>"
                         "<li>通过 <b>文件</b> 菜单添加 Torrent 文件或磁力链接</li>"
                         "<li>在 <b>传输</b> 页查看和管理下载任务</li>"
                         "<li>右键任务可暂停、删除、修改 Tracker 等</li>"
                         "<li>使用 <b>RSS 订阅</b> 页自动追踪新内容</li>"
                         "<li>搜索功能正在升级，当前版本暂时隐藏搜索页</li>"
                         "<li>关闭窗口后程序将最小化到系统托盘</li>"
                         "</ul>"));
    });
  }
  if (aboutAction_ != nullptr) {
    connect(aboutAction_, &QAction::triggered, this, [this]() {
      QMessageBox::about(
          this, QStringLiteral("关于"),
          QStringLiteral("<h3>P2P 文件下载系统</h3>"
                         "<p>版本: 1.0.0</p>"
                         "<p>基于 <b>libtorrent-rasterbar 2.x</b> 和 <b>Qt %1</b> 构建。</p>"
                         "<p>许可证: MIT</p>")
              .arg(QString::fromLatin1(qVersion())));
    });
  }

  auto* selectAllShortcut = new QShortcut(QKeySequence::SelectAll, this);
  connect(selectAllShortcut, &QShortcut::activated, this, [this]() {
    if (taskTable_ != nullptr) {
      taskTable_->selectAll();
      if (taskTable_->rowCount() > 0 && taskTable_->selectionModel() != nullptr) {
        const QModelIndex lastIdx = taskTable_->model()->index(taskTable_->rowCount() - 1, 0);
        taskTable_->selectionModel()->setCurrentIndex(lastIdx, QItemSelectionModel::NoUpdate);
      }
      appendLog(QStringLiteral("已全选所有任务（Ctrl+A）。"));
    }
  });

  if (transferPage_ != nullptr) {
    connect(transferPage_, &pfd::ui::TransferPage::filterChanged, this,
            [this]() { refreshTasks(snapshots_); });
    connect(transferPage_, &pfd::ui::TransferPage::sortChanged, this,
            [this]() { refreshTasks(snapshots_); });
  }

  if (taskTable_ != nullptr) {
    connect(taskTable_, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { showTaskContextMenu(pos); });
  }
}

void MainWindow::openTorrentLinksFromMenu() {
  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("打开 Torrent 链接"));
  dlg.setModal(true);
  dlg.resize(720, 420);
  auto* layout = new QVBoxLayout(&dlg);
  layout->addWidget(new QLabel(
      QStringLiteral("每行一个链接（磁力链接 magnet:?xt=...），可一次添加多个。"), &dlg));
  auto* edit = new QTextEdit(&dlg);
  edit->setPlaceholderText(QStringLiteral("magnet:?xt=urn:btih:...\nmagnet:?xt=urn:btih:..."));
  layout->addWidget(edit, 1);
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  layout->addWidget(buttons);

  if (dlg.exec() != QDialog::Accepted) {
    appendLog(QStringLiteral("已取消打开 Torrent 链接。"));
    return;
  }
  const QStringList lines = edit->toPlainText().split('\n', Qt::SkipEmptyParts);
  int submitted = 0;
  for (const auto& line : lines) {
    const QString uri = line.trimmed();
    if (uri.isEmpty()) {
      continue;
    }
    if (onAddMagnet_) {
      onAddMagnet_(uri, selectedSavePath());
      ++submitted;
    }
  }
  appendLog(QStringLiteral("已提交 Torrent 链接：%1 条").arg(submitted));
}

void MainWindow::openCreateTorrentDialog() {
  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("制作 Torrent"));
  dlg.setModal(true);
  dlg.resize(760, 680);
  auto* root = new QVBoxLayout(&dlg);

  auto* sourceGroup = new QGroupBox(QStringLiteral("选择要共享的文件/文件夹"), &dlg);
  auto* sourceForm = new QFormLayout(sourceGroup);
  auto* sourceRow = new QWidget(sourceGroup);
  auto* sourceRowLayout = new QHBoxLayout(sourceRow);
  sourceRowLayout->setContentsMargins(0, 0, 0, 0);
  sourceRowLayout->setSpacing(8);
  auto* sourcePath = new QLineEdit(QDir::homePath(), sourceRow);
  auto* selectFileBtn = new QPushButton(QStringLiteral("选择文件"), sourceRow);
  auto* selectDirBtn = new QPushButton(QStringLiteral("选择文件夹"), sourceRow);
  sourceRowLayout->addWidget(sourcePath, 1);
  sourceRowLayout->addWidget(selectFileBtn);
  sourceRowLayout->addWidget(selectDirBtn);
  sourceForm->addRow(QStringLiteral("路径："), sourceRow);
  root->addWidget(sourceGroup);

  auto* settingsGroup = new QGroupBox(QStringLiteral("设置"), &dlg);
  auto* settingsForm = new QFormLayout(settingsGroup);
  auto* pieceSize = new QComboBox(settingsGroup);
  pieceSize->addItem(QStringLiteral("自动"), 0);
  pieceSize->addItem(QStringLiteral("128 KiB"), 128 * 1024);
  pieceSize->addItem(QStringLiteral("256 KiB"), 256 * 1024);
  pieceSize->addItem(QStringLiteral("512 KiB"), 512 * 1024);
  pieceSize->addItem(QStringLiteral("1 MiB"), 1024 * 1024);
  pieceSize->addItem(QStringLiteral("2 MiB"), 2 * 1024 * 1024);
  pieceSize->addItem(QStringLiteral("4 MiB"), 4 * 1024 * 1024);
  settingsForm->addRow(QStringLiteral("分块大小："), pieceSize);
  auto* privateTorrentCheck =
      new QCheckBox(QStringLiteral("私有 torrent（不在 DHT 网络上分发）"), settingsGroup);
  settingsForm->addRow(QString(), privateTorrentCheck);
  root->addWidget(settingsGroup);

  auto* fieldsGroup = new QGroupBox(QStringLiteral("字段"), &dlg);
  auto* fieldsForm = new QFormLayout(fieldsGroup);
  auto* trackersEdit = new QTextEdit(fieldsGroup);
  auto* webSeedsEdit = new QTextEdit(fieldsGroup);
  auto* commentEdit = new QTextEdit(fieldsGroup);
  auto* sourceEdit = new QLineEdit(fieldsGroup);
  trackersEdit->setPlaceholderText(QStringLiteral("每行一个 Tracker URL"));
  webSeedsEdit->setPlaceholderText(QStringLiteral("每行一个 Web 种子 URL"));
  fieldsForm->addRow(QStringLiteral("Tracker URL："), trackersEdit);
  fieldsForm->addRow(QStringLiteral("Web 种子 URL："), webSeedsEdit);
  fieldsForm->addRow(QStringLiteral("注释："), commentEdit);
  fieldsForm->addRow(QStringLiteral("源："), sourceEdit);
  root->addWidget(fieldsGroup, 1);

  auto* progress = new QProgressBar(&dlg);
  progress->setRange(0, 100);
  progress->setValue(0);
  root->addWidget(progress);
  auto* progressText = new QLabel(QStringLiteral("进度：0%"), &dlg);
  root->addWidget(progressText);
  auto* openOutputDirCheck = new QCheckBox(QStringLiteral("完成后打开输出目录"), &dlg);
  openOutputDirCheck->setChecked(true);
  root->addWidget(openOutputDirCheck);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
  auto* createBtn = new QPushButton(QStringLiteral("制作 Torrent"), &dlg);
  buttons->addButton(createBtn, QDialogButtonBox::AcceptRole);
  connect(selectFileBtn, &QPushButton::clicked, &dlg, [sourcePath, &dlg]() {
    const QString path =
        QFileDialog::getOpenFileName(&dlg, QStringLiteral("选择文件"), sourcePath->text());
    if (!path.isEmpty()) {
      sourcePath->setText(path);
    }
  });
  connect(selectDirBtn, &QPushButton::clicked, &dlg, [sourcePath, &dlg]() {
    const QString path =
        QFileDialog::getExistingDirectory(&dlg, QStringLiteral("选择文件夹"), sourcePath->text());
    if (!path.isEmpty()) {
      sourcePath->setText(path);
    }
  });
  connect(
      createBtn, &QPushButton::clicked, &dlg,
      [this, &dlg, sourcePath, pieceSize, privateTorrentCheck, trackersEdit, webSeedsEdit,
       commentEdit, sourceEdit, progress, progressText, openOutputDirCheck, createBtn]() {
        const QString src = sourcePath->text().trimmed();
        if (src.isEmpty() || !QFileInfo::exists(src)) {
          QMessageBox::warning(&dlg, QStringLiteral("生成 Torrent"),
                               QStringLiteral("请选择有效的源路径。"));
          return;
        }
        const QString outputPath = QFileDialog::getSaveFileName(
            &dlg, QStringLiteral("保存 Torrent 文件"),
            QFileInfo(src).dir().filePath(QFileInfo(src).baseName() + QStringLiteral(".torrent")),
            QStringLiteral("Torrent 文件 (*.torrent)"));
        if (outputPath.isEmpty()) {
          return;
        }
        progress->setValue(0);
        progressText->setText(QStringLiteral("进度：0%"));
        pfd::core::CreateTorrentRequest req;
        req.source_path = src;
        req.output_torrent_path = outputPath;
        req.piece_size_bytes = pieceSize->currentData().toInt();
        req.private_torrent = privateTorrentCheck->isChecked();
        req.trackers_text = trackersEdit->toPlainText();
        req.web_seeds_text = webSeedsEdit->toPlainText();
        req.comment = commentEdit->toPlainText();
        req.source = sourceEdit->text();
        createBtn->setEnabled(false);
        progressText->setText(QStringLiteral("进度：准备生成..."));
        const bool openDirAfter = openOutputDirCheck->isChecked();
        const QPointer<QDialog> dlgGuard(&dlg);
        const QPointer<MainWindow> selfGuard(this);
        std::thread([selfGuard, req, outputPath, progress, progressText, createBtn, openDirAfter,
                     dlgGuard]() {
          const auto result =
              pfd::core::TorrentCreator::create(req, [progress, progressText](int value) {
                const int p = std::clamp(value, 0, 100);
                QMetaObject::invokeMethod(progress, [progress, progressText, p]() {
                  progress->setValue(p);
                  progressText->setText(QStringLiteral("进度：%1%").arg(p));
                });
              });
          QMetaObject::invokeMethod(progress, [selfGuard, result, outputPath, progressText,
                                               createBtn, openDirAfter, dlgGuard]() {
            if (selfGuard.isNull()) {
              return;
            }
            createBtn->setEnabled(true);
            if (dlgGuard.isNull()) {
              return;
            }
            if (!result.ok) {
              QMessageBox::warning(dlgGuard.data(), QStringLiteral("生成失败"), result.error);
              selfGuard->appendLog(QStringLiteral("生成 Torrent 失败：%1").arg(result.error));
              return;
            }
            progressText->setText(QStringLiteral("进度：100%（完成）"));
            QMessageBox::information(dlgGuard.data(), QStringLiteral("生成成功"),
                                     QStringLiteral("已生成 Torrent：%1").arg(outputPath));
            selfGuard->appendLog(QStringLiteral("已生成 Torrent：%1").arg(outputPath));
            if (openDirAfter) {
              QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(outputPath).absolutePath()));
            }
          });
        }).detach();
      });
  connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  root->addWidget(buttons);
  dlg.exec();
}

void MainWindow::openCookieManagerDialog() {
  QDialog dlg(this);
  dlg.setWindowTitle(QStringLiteral("管理 Cookies"));
  dlg.setModal(true);
  dlg.resize(760, 520);
  auto* root = new QVBoxLayout(&dlg);
  auto* table = new QTableWidget(0, 3, &dlg);
  table->setHorizontalHeaderLabels(
      {QStringLiteral("域名（* 表示全局）"), QStringLiteral("Cookie 名"), QStringLiteral("值")});
  table->horizontalHeader()->setStretchLastSection(true);
  root->addWidget(table, 1);

  auto* filterRow = new QHBoxLayout();
  auto* filterEdit = new QLineEdit(&dlg);
  filterEdit->setPlaceholderText(QStringLiteral("筛选域名/Cookie 名/值"));
  filterRow->addWidget(new QLabel(QStringLiteral("筛选："), &dlg));
  filterRow->addWidget(filterEdit, 1);
  root->addLayout(filterRow);
  auto* duplicateHint = new QLabel(QString(), &dlg);
  duplicateHint->setProperty("class", QStringLiteral("sectionHint"));
  root->addWidget(duplicateHint);

  const auto initial = pfd::core::ConfigService::loadAppSettings();
  auto appendCookieRow = [table](const QString& domain, const QString& pairText) {
    const int pos = pairText.indexOf('=');
    if (pos <= 0)
      return;
    const int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(domain));
    table->setItem(row, 1, new QTableWidgetItem(pairText.left(pos).trimmed()));
    table->setItem(row, 2, new QTableWidgetItem(pairText.mid(pos + 1).trimmed()));
  };
  for (const QString& entryRaw : initial.http_cookie_header.split(';', Qt::SkipEmptyParts)) {
    appendCookieRow(QStringLiteral("*"), entryRaw.trimmed());
  }
  for (const QString& lineRaw : initial.http_cookie_rules.split('\n', Qt::SkipEmptyParts)) {
    const QString line = lineRaw.trimmed();
    const int tabPos = line.indexOf('\t');
    if (tabPos <= 0 || tabPos >= line.size() - 1) {
      continue;
    }
    appendCookieRow(line.left(tabPos).trimmed(), line.mid(tabPos + 1).trimmed());
  }

  auto* actionRow = new QHBoxLayout();
  auto* addBtn = new QPushButton(QStringLiteral("新增"), &dlg);
  auto* removeBtn = new QPushButton(QStringLiteral("删除选中"), &dlg);
  auto* importBtn = new QPushButton(QStringLiteral("导入"), &dlg);
  auto* exportBtn = new QPushButton(QStringLiteral("导出"), &dlg);
  actionRow->addWidget(addBtn);
  actionRow->addWidget(removeBtn);
  actionRow->addWidget(importBtn);
  actionRow->addWidget(exportBtn);
  actionRow->addStretch(1);
  root->addLayout(actionRow);
  auto refreshDuplicateHighlight = [table, duplicateHint]() {
    QSignalBlocker blocker(table);
    for (int row = 0; row < table->rowCount(); ++row) {
      for (int col = 0; col < table->columnCount(); ++col) {
        if (table->item(row, col) == nullptr) {
          table->setItem(row, col, new QTableWidgetItem());
        }
        table->item(row, col)->setBackground(QBrush());
      }
    }
    QHash<QString, QVector<int>> keyRows;
    for (int row = 0; row < table->rowCount(); ++row) {
      const QString domain = table->item(row, 0) != nullptr
                                 ? table->item(row, 0)->text().trimmed().toLower()
                                 : QStringLiteral("*");
      const QString name = table->item(row, 1) != nullptr
                               ? table->item(row, 1)->text().trimmed().toLower()
                               : QString();
      if (name.isEmpty()) {
        continue;
      }
      keyRows[QStringLiteral("%1|%2").arg(domain.isEmpty() ? QStringLiteral("*") : domain, name)]
          .push_back(row);
    }
    int duplicateCount = 0;
    const QColor duplicateColor(255, 234, 234);
    for (auto it = keyRows.constBegin(); it != keyRows.constEnd(); ++it) {
      if (it.value().size() <= 1) {
        continue;
      }
      duplicateCount += it.value().size();
      for (const int row : it.value()) {
        for (int col = 0; col < table->columnCount(); ++col) {
          table->item(row, col)->setBackground(duplicateColor);
        }
      }
    }
    if (duplicateCount > 0) {
      duplicateHint->setText(
          QStringLiteral("检测到 %1 行重复项（相同域名 + Cookie 名），保存将被阻止。")
              .arg(duplicateCount));
    } else {
      duplicateHint->setText(QStringLiteral("未检测到重复项。"));
    }
  };
  QObject::connect(table, &QTableWidget::itemChanged, &dlg,
                   [refreshDuplicateHighlight](QTableWidgetItem*) { refreshDuplicateHighlight(); });
  connect(addBtn, &QPushButton::clicked, &dlg, [table]() {
    const int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(QStringLiteral("*")));
    table->setItem(row, 1, new QTableWidgetItem());
    table->setItem(row, 2, new QTableWidgetItem());
    table->setCurrentCell(row, 1);
  });
  connect(removeBtn, &QPushButton::clicked, &dlg, [table]() {
    const auto rows = table->selectionModel()->selectedRows();
    for (int i = rows.size() - 1; i >= 0; --i) {
      table->removeRow(rows.at(i).row());
    }
  });
  connect(filterEdit, &QLineEdit::textChanged, &dlg, [table](const QString& text) {
    const QString key = text.trimmed().toLower();
    for (int row = 0; row < table->rowCount(); ++row) {
      const QString domain =
          table->item(row, 0) != nullptr ? table->item(row, 0)->text().toLower() : QString();
      const QString name =
          table->item(row, 1) != nullptr ? table->item(row, 1)->text().toLower() : QString();
      const QString value =
          table->item(row, 2) != nullptr ? table->item(row, 2)->text().toLower() : QString();
      const bool visible =
          key.isEmpty() || domain.contains(key) || name.contains(key) || value.contains(key);
      table->setRowHidden(row, !visible);
    }
  });
  connect(importBtn, &QPushButton::clicked, &dlg, [table, &dlg]() {
    const QString path =
        QFileDialog::getOpenFileName(&dlg, QStringLiteral("导入 Cookies"), QString(),
                                     QStringLiteral("文本文件 (*.txt *.tsv);;所有文件 (*)"));
    if (path.isEmpty()) {
      return;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QMessageBox::warning(&dlg, QStringLiteral("导入失败"),
                           QStringLiteral("无法读取文件：%1").arg(path));
      return;
    }
    const QString content = QString::fromUtf8(f.readAll());
    f.close();
    for (const QString& raw : content.split('\n', Qt::SkipEmptyParts)) {
      const QString line = raw.trimmed();
      if (line.isEmpty() || line.startsWith('#')) {
        continue;
      }
      QString domain = QStringLiteral("*");
      QString name;
      QString value;
      const QStringList cols = line.split('\t');
      if (cols.size() >= 3) {
        domain = cols[0].trimmed();
        name = cols[1].trimmed();
        value = cols[2].trimmed();
      } else {
        const int eq = line.indexOf('=');
        if (eq <= 0) {
          continue;
        }
        name = line.left(eq).trimmed();
        value = line.mid(eq + 1).trimmed();
      }
      if (name.isEmpty()) {
        continue;
      }
      const int row = table->rowCount();
      table->insertRow(row);
      table->setItem(row, 0, new QTableWidgetItem(domain.isEmpty() ? QStringLiteral("*") : domain));
      table->setItem(row, 1, new QTableWidgetItem(name));
      table->setItem(row, 2, new QTableWidgetItem(value));
    }
  });
  connect(exportBtn, &QPushButton::clicked, &dlg, [table, &dlg]() {
    const QString path = QFileDialog::getSaveFileName(
        &dlg, QStringLiteral("导出 Cookies"), QStringLiteral("cookies.tsv"),
        QStringLiteral("TSV 文件 (*.tsv);;文本文件 (*.txt)"));
    if (path.isEmpty()) {
      return;
    }
    QStringList lines;
    lines.push_back(QStringLiteral("# domain<TAB>name<TAB>value"));
    for (int row = 0; row < table->rowCount(); ++row) {
      const QString domain = table->item(row, 0) != nullptr ? table->item(row, 0)->text().trimmed()
                                                            : QStringLiteral("*");
      const QString name =
          table->item(row, 1) != nullptr ? table->item(row, 1)->text().trimmed() : QString();
      const QString value =
          table->item(row, 2) != nullptr ? table->item(row, 2)->text().trimmed() : QString();
      if (name.isEmpty()) {
        continue;
      }
      lines.push_back(QStringLiteral("%1\t%2\t%3")
                          .arg(domain.isEmpty() ? QStringLiteral("*") : domain, name, value));
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      QMessageBox::warning(&dlg, QStringLiteral("导出失败"),
                           QStringLiteral("无法写入文件：%1").arg(path));
      return;
    }
    f.write(lines.join('\n').toUtf8());
    f.close();
  });
  refreshDuplicateHighlight();

  root->addWidget(new QLabel(QStringLiteral("提示：域名可填写 example.com；* 表示全局。"), &dlg));
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, &dlg);
  connect(buttons, &QDialogButtonBox::accepted, &dlg, [this, table, &dlg]() {
    QStringList globalPairs;
    QStringList domainRules;
    QSet<QString> dedupKeys;
    for (int row = 0; row < table->rowCount(); ++row) {
      const QString domain = table->item(row, 0) != nullptr ? table->item(row, 0)->text().trimmed()
                                                            : QStringLiteral("*");
      const QString name =
          table->item(row, 1) != nullptr ? table->item(row, 1)->text().trimmed() : QString();
      const QString value =
          table->item(row, 2) != nullptr ? table->item(row, 2)->text().trimmed() : QString();
      if (name.isEmpty()) {
        continue;
      }
      const QString dedupKey = QStringLiteral("%1|%2").arg(domain.toLower(), name.toLower());
      if (dedupKeys.contains(dedupKey)) {
        QMessageBox::warning(
            &dlg, QStringLiteral("保存失败"),
            QStringLiteral("检测到重复项：域名=%1，Cookie 名=%2").arg(domain, name));
        return;
      }
      dedupKeys.insert(dedupKey);
      const QString pair = QStringLiteral("%1=%2").arg(name, value);
      if (domain.isEmpty() || domain == QStringLiteral("*")) {
        globalPairs.push_back(pair);
      } else {
        domainRules.push_back(QStringLiteral("%1\t%2").arg(domain.toLower(), pair));
      }
    }

    auto combined = pfd::core::ConfigService::loadCombinedSettings();
    combined.app.http_cookie_header = globalPairs.join(QStringLiteral("; "));
    combined.app.http_cookie_rules = domainRules.join('\n');
    const auto result = pfd::core::ConfigService::saveCombinedSettings(combined);
    if (!result.ok) {
      QMessageBox::warning(&dlg, QStringLiteral("保存失败"),
                           QStringLiteral("Cookies 保存失败：%1").arg(result.message));
      return;
    }
    appendLog(QStringLiteral("Cookies 已更新：全局 %1 条，域名规则 %2 条")
                  .arg(globalPairs.size())
                  .arg(domainRules.size()));
    emit settingsChanged();
    dlg.accept();
  });
  connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
  root->addWidget(buttons);
  dlg.exec();
}

void MainWindow::setOnAddMagnet(std::function<void(const QString&, const QString&)> onAddMagnet) {
  onAddMagnet_ = std::move(onAddMagnet);
}

void MainWindow::setOnAddTorrentFile(
    std::function<void(const QString&, const QString&)> onAddTorrentFile) {
  onAddTorrentFile_ = std::move(onAddTorrentFile);
}

void MainWindow::setOnAddTorrentFileRequest(
    std::function<void(const AddTorrentFileRequest&)> onAddTorrentFileRequest) {
  onAddTorrentFileRequest_ = std::move(onAddTorrentFileRequest);
}

void MainWindow::setOnPauseTask(std::function<void(const pfd::base::TaskId&)> onPauseTask) {
  onPauseTask_ = std::move(onPauseTask);
}

void MainWindow::setOnResumeTask(std::function<void(const pfd::base::TaskId&)> onResumeTask) {
  onResumeTask_ = std::move(onResumeTask);
}

void MainWindow::setOnRemoveTask(std::function<void(const pfd::base::TaskId&, bool)> onRemoveTask) {
  onRemoveTask_ = std::move(onRemoveTask);
}

void MainWindow::setOnMoveTask(
    std::function<void(const pfd::base::TaskId&, const QString&)> onMoveTask) {
  onMoveTask_ = std::move(onMoveTask);
}

void MainWindow::setOnRenameTask(
    std::function<void(const pfd::base::TaskId&, const QString&)> onRenameTask) {
  onRenameTask_ = std::move(onRenameTask);
}

void MainWindow::setOnEditTrackers(
    std::function<void(const pfd::base::TaskId&, const QStringList&)> onEditTrackers) {
  onEditTrackers_ = std::move(onEditTrackers);
}

void MainWindow::setOnQueryTaskTrackers(
    std::function<QStringList(const pfd::base::TaskId&)> onQueryTaskTrackers) {
  onQueryTaskTrackers_ = std::move(onQueryTaskTrackers);
}

void MainWindow::setOnCategoryChanged(
    std::function<void(const pfd::base::TaskId&, const QString&)> onCategoryChanged) {
  onCategoryChanged_ = std::move(onCategoryChanged);
}

void MainWindow::setOnTagsChanged(
    std::function<void(const pfd::base::TaskId&, const QStringList&)> onTagsChanged) {
  onTagsChanged_ = std::move(onTagsChanged);
}

void MainWindow::setOnDefaultTrackersChanged(
    std::function<void(bool, const QStringList&)> onDefaultTrackersChanged) {
  onDefaultTrackersChanged_ = std::move(onDefaultTrackersChanged);
}

void MainWindow::setOnQueryDefaultTrackers(std::function<QStringList()> onQueryDefaultTrackers) {
  onQueryDefaultTrackers_ = std::move(onQueryDefaultTrackers);
}

void MainWindow::setOnQueryDefaultTrackerEnabled(
    std::function<bool()> onQueryDefaultTrackerEnabled) {
  onQueryDefaultTrackerEnabled_ = std::move(onQueryDefaultTrackerEnabled);
}

void MainWindow::setOnSeedPolicyChanged(std::function<void(bool, double)> onSeedPolicyChanged) {
  onSeedPolicyChanged_ = std::move(onSeedPolicyChanged);
}

void MainWindow::setOnSetTaskConnectionsLimit(
    std::function<void(const pfd::base::TaskId&, int)> onSetTaskConnectionsLimit) {
  onSetTaskConnectionsLimit_ = std::move(onSetTaskConnectionsLimit);
}

void MainWindow::setOnForceStartTask(
    std::function<void(const pfd::base::TaskId&)> onForceStartTask) {
  onForceStartTask_ = std::move(onForceStartTask);
}

void MainWindow::setOnForceRecheckTask(
    std::function<void(const pfd::base::TaskId&)> onForceRecheckTask) {
  onForceRecheckTask_ = std::move(onForceRecheckTask);
}

void MainWindow::setOnForceReannounceTask(
    std::function<void(const pfd::base::TaskId&)> onForceReannounceTask) {
  onForceReannounceTask_ = std::move(onForceReannounceTask);
}

void MainWindow::setOnSetSequentialDownload(
    std::function<void(const pfd::base::TaskId&, bool)> onSetSequentialDownload) {
  onSetSequentialDownload_ = std::move(onSetSequentialDownload);
}

void MainWindow::setOnSetAutoManagedTask(
    std::function<void(const pfd::base::TaskId&, bool)> onSetAutoManagedTask) {
  onSetAutoManagedTask_ = std::move(onSetAutoManagedTask);
}

void MainWindow::setOnFirstLastPiecePriority(
    std::function<void(const pfd::base::TaskId&, bool)> onFirstLastPiecePriority) {
  onFirstLastPiecePriority_ = std::move(onFirstLastPiecePriority);
}

void MainWindow::setOnPreviewFile(std::function<void(const pfd::base::TaskId&)> onPreviewFile) {
  onPreviewFile_ = std::move(onPreviewFile);
}

void MainWindow::setOnQueuePositionUp(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionUp_ = std::move(cb);
}

void MainWindow::setOnQueuePositionDown(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionDown_ = std::move(cb);
}

void MainWindow::setOnQueuePositionTop(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionTop_ = std::move(cb);
}

void MainWindow::setOnQueuePositionBottom(std::function<void(const pfd::base::TaskId&)> cb) {
  onQueuePositionBottom_ = std::move(cb);
}

void MainWindow::setOnQueryCopyPayload(
    std::function<CopyPayload(const pfd::base::TaskId&)> onQueryCopyPayload) {
  onQueryCopyPayload_ = std::move(onQueryCopyPayload);
}

void MainWindow::setOnQueryTaskFiles(
    std::function<std::vector<pfd::core::TaskFileEntryDto>(const pfd::base::TaskId&)> cb) {
  onQueryTaskFiles_ = std::move(cb);
  syncContentHandlers();
}

void MainWindow::setOnSetTaskFilePriority(
    std::function<void(const pfd::base::TaskId&, const std::vector<int>&,
                       pfd::core::TaskFilePriorityLevel)>
        cb) {
  onSetTaskFilePriority_ = std::move(cb);
  syncContentHandlers();
}

void MainWindow::setOnRenameTaskFileOrFolder(
    std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> cb) {
  onRenameTaskFileOrFolder_ = std::move(cb);
  syncContentHandlers();
}

void MainWindow::setOnQueryTaskTrackerSnapshot(
    std::function<pfd::core::TaskTrackerSnapshotDto(const pfd::base::TaskId&)> cb) {
  onQueryTaskTrackerSnapshot_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnAddTaskTracker(
    std::function<void(const pfd::base::TaskId&, const QString&)> cb) {
  onAddTaskTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnEditTaskTracker(
    std::function<void(const pfd::base::TaskId&, const QString&, const QString&)> cb) {
  onEditTaskTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnRemoveTaskTracker(
    std::function<void(const pfd::base::TaskId&, const QString&)> cb) {
  onRemoveTaskTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnForceReannounceTracker(
    std::function<void(const pfd::base::TaskId&, const QString&)> cb) {
  onForceReannounceTracker_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnForceReannounceAllTrackers(std::function<void(const pfd::base::TaskId&)> cb) {
  onForceReannounceAllTrackers_ = std::move(cb);
  syncTrackerHandlers();
}

void MainWindow::setOnQueryTaskPeers(
    std::function<std::vector<pfd::core::TaskPeerDto>(const pfd::base::TaskId&)> cb) {
  onQueryTaskPeers_ = std::move(cb);
  if (transferPage_ != nullptr) {
    transferPage_->setPeerHandlers(onQueryTaskPeers_);
  }
}

void MainWindow::setOnQueryTaskWebSeeds(
    std::function<std::vector<pfd::core::TaskWebSeedDto>(const pfd::base::TaskId&)> cb) {
  onQueryTaskWebSeeds_ = std::move(cb);
  if (transferPage_ != nullptr) {
    transferPage_->setHttpSourceHandlers(onQueryTaskWebSeeds_);
  }
}

void MainWindow::setSearchDataSources(SearchTab::QuerySnapshotsFn snapshotsFn,
                                      SearchTab::QueryRssItemsFn rssItemsFn) {
  if (searchTab_ != nullptr) {
    searchTab_->setQuerySnapshotsFn(std::move(snapshotsFn));
    searchTab_->setQueryRssItemsFn(std::move(rssItemsFn));
  }
}

void MainWindow::setSearchRequestHeaders(const SearchTab::RequestHeaders& headers) {
  if (searchTab_ != nullptr) {
    searchTab_->setRequestHeaders(headers);
  }
}

void MainWindow::syncContentHandlers() {
  if (transferPage_ == nullptr)
    return;
  transferPage_->setContentHandlers(onQueryTaskFiles_, onSetTaskFilePriority_,
                                    onRenameTaskFileOrFolder_);
}

void MainWindow::syncTrackerHandlers() {
  if (transferPage_ == nullptr)
    return;
  transferPage_->setTrackerHandlers(onQueryTaskTrackerSnapshot_, onAddTaskTracker_,
                                    onEditTaskTracker_, onRemoveTaskTracker_,
                                    onForceReannounceTracker_, onForceReannounceAllTrackers_);
}

void MainWindow::appendLog(const QString& msg) {
  LOG_INFO(QStringLiteral("[ui] %1").arg(msg));
  if (logView_ != nullptr) {
    const auto ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    logView_->append(QStringLiteral("[%1] %2").arg(ts, msg));
  }
}

QString MainWindow::selectedSavePath() const {
  const QString p = pfd::core::ConfigService::loadAppSettings().default_download_dir;
  return p.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) : p;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if (taskTable_ != nullptr && event != nullptr && event->type() == QEvent::MouseButtonPress) {
    const auto isInsideTaskTable = [this](const QObject* o) {
      for (const QObject* cur = o; cur != nullptr; cur = cur->parent()) {
        if (cur == taskTable_ || cur == taskTable_->viewport()) {
          return true;
        }
      }
      return false;
    };

    if (isInsideTaskTable(watched)) {
      if (watched == taskTable_->viewport()) {
        const auto* me = static_cast<QMouseEvent*>(event);
        const QModelIndex idx = taskTable_->indexAt(me->pos());
        if (!idx.isValid()) {
          if (transferPage_ != nullptr) {
            transferPage_->enterMemoryModeFromCurrentSelection();
          }
        }
      }
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

}  // namespace pfd::ui
