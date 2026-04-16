#include "ui/add_torrent_dialog.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

#include <libtorrent/error_code.hpp>
#include <libtorrent/info_hash.hpp>
#include <libtorrent/torrent_info.hpp>

#include "ui/input_ime_utils.h"

namespace pfd::ui {

namespace {

QString formatBytes(qint64 bytes) {
  const double b = static_cast<double>(bytes);
  const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
  int u = 0;
  double v = b;
  while (v >= 1024.0 && u < 4) {
    v /= 1024.0;
    ++u;
  }
  return QStringLiteral("%1 %2").arg(v, 0, 'f', u == 0 ? 0 : 2).arg(QString::fromLatin1(units[u]));
}

QString toHexV1(const libtorrent::info_hash_t& ih) {
  if (!ih.has_v1()) {
    return QString();
  }
  std::ostringstream oss;
  oss << ih.v1;
  return QString::fromStdString(oss.str());
}

QString toHexV2(const libtorrent::info_hash_t& ih) {
  if (!ih.has_v2()) {
    return QString();
  }
  std::ostringstream oss;
  oss << ih.v2;
  return QString::fromStdString(oss.str());
}

}  // namespace

std::optional<AddTorrentDialog::Result>
AddTorrentDialog::runForTorrentFile(QWidget* parent, const QString& torrentFilePath,
                                    const QString& defaultSavePath) {
  AddTorrentDialog dlg(parent);
  if (!dlg.loadTorrentFile(torrentFilePath)) {
    return std::nullopt;
  }
  if (dlg.savePathEdit_ != nullptr) {
    dlg.savePathEdit_->setText(defaultSavePath);
  }
  if (dlg.exec() != QDialog::Accepted) {
    return std::nullopt;
  }
  return dlg.result();
}

std::optional<AddTorrentDialog::Result>
AddTorrentDialog::runForMagnetMetadata(QWidget* parent, const MagnetInput& in,
                                       const QString& defaultSavePath) {
  AddTorrentDialog dlg(parent);
  if (!dlg.loadMagnetMetadata(in)) {
    return std::nullopt;
  }
  if (dlg.savePathEdit_ != nullptr) {
    dlg.savePathEdit_->setText(defaultSavePath);
  }
  if (dlg.exec() != QDialog::Accepted) {
    return std::nullopt;
  }
  return dlg.result();
}

AddTorrentDialog::AddTorrentDialog(QWidget* parent) : QDialog(parent) {
  setModal(true);
  resize(1080, 680);
  buildLayout();
  bindSignals();
}

bool AddTorrentDialog::loadTorrentFile(const QString& torrentFilePath) {
  libtorrent::error_code ec;
  libtorrent::torrent_info ti(torrentFilePath.toStdString(), ec);
  if (ec) {
    return false;
  }

  torrentName_ = QString::fromStdString(ti.name()).trimmed();
  if (torrentName_.isEmpty()) {
    torrentName_ = QFileInfo(torrentFilePath).completeBaseName().trimmed();
  }
  if (torrentName_.isEmpty()) {
    torrentName_ = QStringLiteral("未命名任务");
  }
  setWindowTitle(torrentName_);
  if (title_ != nullptr) {
    title_->setText(torrentName_);
  }

  totalBytes_ = 0;
  fileSizes_.clear();
  const auto& fs = ti.files();
  const int n = fs.num_files();
  fileSizes_.reserve(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    const auto sz = static_cast<qint64>(fs.file_size(libtorrent::file_index_t{i}));
    fileSizes_.push_back(sz);
    totalBytes_ += sz;
  }

  const auto ih = ti.info_hashes();
  hashV1_ = toHexV1(ih);
  hashV2_ = toHexV2(ih);
  comment_ = QString::fromStdString(ti.comment());

  // creation_date() returns time_t in seconds; 0 means missing
  creationDate_ = static_cast<qint64>(ti.creation_date());

  applyMetaInfoToUi();
  std::vector<QString> filePaths;
  filePaths.reserve(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    const auto idx = libtorrent::file_index_t{i};
    filePaths.push_back(QString::fromStdString(fs.file_path(idx)));
  }
  rebuildFileTree(filePaths);

  rebuildStats();
  return true;
}

bool AddTorrentDialog::loadMagnetMetadata(const MagnetInput& in) {
  if (in.name.trimmed().isEmpty() || in.filePaths.size() != in.fileSizes.size()) {
    return false;
  }

  torrentName_ = in.name;
  setWindowTitle(torrentName_);
  if (title_ != nullptr) {
    title_->setText(torrentName_);
  }

  totalBytes_ = in.totalBytes;
  creationDate_ = in.creationDate;
  comment_ = in.comment;
  hashV1_ = in.infoHashV1;
  hashV2_ = in.infoHashV2;
  fileSizes_ = in.fileSizes;

  applyMetaInfoToUi();
  rebuildFileTree(in.filePaths);

  rebuildStats();
  return true;
}

void AddTorrentDialog::applyMetaInfoToUi() {
  if (infoSize_ != nullptr) {
    infoSize_->setText(QStringLiteral("%1（已勾选：0）").arg(formatBytes(totalBytes_)));
  }
  if (infoDate_ != nullptr) {
    infoDate_->setText(creationDate_ > 0 ? QDateTime::fromSecsSinceEpoch(creationDate_)
                                               .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                         : QStringLiteral("--"));
  }
  if (infoHashV1_ != nullptr)
    infoHashV1_->setText(hashV1_.isEmpty() ? QStringLiteral("--") : hashV1_);
  if (infoHashV2_ != nullptr)
    infoHashV2_->setText(hashV2_.isEmpty() ? QStringLiteral("--") : hashV2_);
  if (infoComment_ != nullptr)
    infoComment_->setText(comment_.isEmpty() ? QStringLiteral("--") : comment_);
}

void AddTorrentDialog::rebuildFileTree(const std::vector<QString>& filePaths) {
  if (fileTree_ == nullptr)
    return;
  fileTree_->clear();
  fileTree_->setColumnCount(3);
  fileTree_->setHeaderLabels(
      {QStringLiteral("文件"), QStringLiteral("大小"), QStringLiteral("下载")});

  const int n = static_cast<int>(filePaths.size());
  for (int i = 0; i < n; ++i) {
    auto* item = new QTreeWidgetItem(fileTree_);
    item->setText(0, filePaths[static_cast<size_t>(i)]);
    item->setText(1, formatBytes(fileSizes_[static_cast<size_t>(i)]));
    item->setCheckState(2, Qt::Checked);
    item->setData(0, Qt::UserRole, i);
  }
  fileTree_->resizeColumnToContents(0);
  fileTree_->resizeColumnToContents(1);
}

void AddTorrentDialog::buildLayout() {
  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(12, 12, 12, 12);
  root->setSpacing(10);

  title_ = new QLabel(QStringLiteral(""), this);
  title_->setStyleSheet(QStringLiteral("font-size:18px;font-weight:800;color:#1f2d3d;"));
  root->addWidget(title_);

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);

  // left panel
  auto* left = new QWidget(splitter);
  auto* leftLayout = new QVBoxLayout(left);
  leftLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->setSpacing(10);

  auto* infoGroup = new QGroupBox(QStringLiteral("Torrent 信息"), left);
  auto* infoForm = new QFormLayout(infoGroup);
  infoSize_ = new QLabel(QStringLiteral("--"), infoGroup);
  infoDate_ = new QLabel(QStringLiteral("--"), infoGroup);
  infoHashV1_ = new QLabel(QStringLiteral("--"), infoGroup);
  infoHashV2_ = new QLabel(QStringLiteral("--"), infoGroup);
  infoComment_ = new QLabel(QStringLiteral("--"), infoGroup);
  infoComment_->setWordWrap(true);
  infoForm->addRow(QStringLiteral("大小"), infoSize_);
  infoForm->addRow(QStringLiteral("日期"), infoDate_);
  infoForm->addRow(QStringLiteral("v1"), infoHashV1_);
  infoForm->addRow(QStringLiteral("v2"), infoHashV2_);
  infoForm->addRow(QStringLiteral("注释"), infoComment_);
  leftLayout->addWidget(infoGroup);

  auto* pathGroup = new QGroupBox(QStringLiteral("保存路径"), left);
  auto* pathGrid = new QGridLayout(pathGroup);
  savePathEdit_ = new QLineEdit(pathGroup);
  browseSavePathBtn_ = new QPushButton(QStringLiteral("浏览"), pathGroup);
  pathGrid->addWidget(new QLabel(QStringLiteral("路径"), pathGroup), 0, 0);
  pathGrid->addWidget(savePathEdit_, 0, 1);
  pathGrid->addWidget(browseSavePathBtn_, 0, 2);

  useIncompleteCheck_ =
      new QCheckBox(QStringLiteral("对不完整的 Torrent 使用另一个路径"), pathGroup);
  incompletePathEdit_ = new QLineEdit(pathGroup);
  browseIncompleteBtn_ = new QPushButton(QStringLiteral("浏览"), pathGroup);
  pathGrid->addWidget(useIncompleteCheck_, 1, 0, 1, 3);
  pathGrid->addWidget(new QLabel(QStringLiteral("不完整路径"), pathGroup), 2, 0);
  pathGrid->addWidget(incompletePathEdit_, 2, 1);
  pathGrid->addWidget(browseIncompleteBtn_, 2, 2);
  leftLayout->addWidget(pathGroup);

  auto* optGroup = new QGroupBox(QStringLiteral("Torrent 选项"), left);
  auto* optForm = new QFormLayout(optGroup);
  categoryEdit_ = new QLineEdit(optGroup);
  tagsEdit_ = new QLineEdit(optGroup);
  tagsEdit_->setPlaceholderText(QStringLiteral("逗号分隔，例如：movie,linux"));
  layoutBox_ = new QComboBox(optGroup);
  layoutBox_->addItem(QStringLiteral("原始"), static_cast<int>(ContentLayout::kOriginal));
  layoutBox_->addItem(QStringLiteral("创建子文件夹"),
                      static_cast<int>(ContentLayout::kCreateSubfolder));
  layoutBox_->addItem(QStringLiteral("不创建子文件夹"),
                      static_cast<int>(ContentLayout::kNoSubfolder));
  startCheck_ = new QCheckBox(QStringLiteral("开始 Torrent"), optGroup);
  startCheck_->setChecked(true);
  stopConditionBox_ = new QComboBox(optGroup);
  stopConditionBox_->addItem(QStringLiteral("无"), 0);
  stopConditionBox_->addItem(QStringLiteral("文件已被检查"), 1);
  sequentialCheck_ = new QCheckBox(QStringLiteral("按顺序下载"), optGroup);
  firstLastCheck_ = new QCheckBox(QStringLiteral("先下载首尾文件块"), optGroup);
  skipHashCheck_ = new QCheckBox(QStringLiteral("跳过哈希校验"), optGroup);
  addTopQueueCheck_ = new QCheckBox(QStringLiteral("添加到队列顶部"), optGroup);
  optForm->addRow(QStringLiteral("分类"), categoryEdit_);
  optForm->addRow(QStringLiteral("标签"), tagsEdit_);
  optForm->addRow(QStringLiteral("内容布局"), layoutBox_);
  optForm->addRow(QStringLiteral("开始"), startCheck_);
  optForm->addRow(QStringLiteral("停止条件"), stopConditionBox_);
  optForm->addRow(QStringLiteral(""), sequentialCheck_);
  optForm->addRow(QStringLiteral(""), firstLastCheck_);
  optForm->addRow(QStringLiteral(""), skipHashCheck_);
  optForm->addRow(QStringLiteral(""), addTopQueueCheck_);
  leftLayout->addWidget(optGroup);
  leftLayout->addStretch(1);

  // right panel
  auto* right = new QWidget(splitter);
  auto* rightLayout = new QVBoxLayout(right);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(10);

  auto* topRow = new QHBoxLayout();
  filterEdit_ = new QLineEdit(right);
  filterEdit_->setPlaceholderText(QStringLiteral("过滤文件（按名称包含匹配）"));
  selectAllBtn_ = new QPushButton(QStringLiteral("全选"), right);
  selectNoneBtn_ = new QPushButton(QStringLiteral("全不选"), right);
  topRow->addWidget(filterEdit_, 1);
  topRow->addWidget(selectAllBtn_);
  topRow->addWidget(selectNoneBtn_);
  rightLayout->addLayout(topRow);

  fileTree_ = new QTreeWidget(right);
  fileTree_->setUniformRowHeights(true);
  rightLayout->addWidget(fileTree_, 1);

  selectedSizeLabel_ = new QLabel(QStringLiteral("已勾选：0"), right);
  rightLayout->addWidget(selectedSizeLabel_);

  splitter->addWidget(left);
  splitter->addWidget(right);
  splitter->setStretchFactor(0, 2);
  splitter->setStretchFactor(1, 3);
  root->addWidget(splitter, 1);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);
}

void AddTorrentDialog::bindSignals() {
  connect(browseSavePathBtn_, &QPushButton::clicked, this, [this]() {
    const QString base = savePathEdit_->text().trimmed();
    const QString dir =
        QFileDialog::getExistingDirectory(this, QStringLiteral("选择保存路径"), base);
    if (!dir.isEmpty())
      savePathEdit_->setText(dir);
  });
  connect(browseIncompleteBtn_, &QPushButton::clicked, this, [this]() {
    const QString base = incompletePathEdit_->text().trimmed();
    const QString dir =
        QFileDialog::getExistingDirectory(this, QStringLiteral("选择不完整路径"), base);
    if (!dir.isEmpty())
      incompletePathEdit_->setText(dir);
  });
  connect(useIncompleteCheck_, &QCheckBox::toggled, this, [this](bool on) {
    incompletePathEdit_->setEnabled(on);
    browseIncompleteBtn_->setEnabled(on);
    const QString style = on ? QString() : QStringLiteral("color:#9aa3af;");
    incompletePathEdit_->setStyleSheet(style);
  });
  useIncompleteCheck_->setChecked(false);
  incompletePathEdit_->setEnabled(false);
  browseIncompleteBtn_->setEnabled(false);
  incompletePathEdit_->setStyleSheet(QStringLiteral("color:#9aa3af;"));

  connectImeAwareLineEditApply(this, filterEdit_, 100,
                               [this]() { applyFilter(filterEdit_->text()); });
  connect(selectAllBtn_, &QPushButton::clicked, this, [this]() { setAllWanted(true); });
  connect(selectNoneBtn_, &QPushButton::clicked, this, [this]() { setAllWanted(false); });
  connect(fileTree_, &QTreeWidget::itemChanged, this,
          [this](QTreeWidgetItem*, int) { rebuildStats(); });
}

void AddTorrentDialog::applyFilter(const QString& text) {
  const QString kw = text.trimmed();
  if (fileTree_ == nullptr)
    return;
  for (int i = 0; i < fileTree_->topLevelItemCount(); ++i) {
    auto* it = fileTree_->topLevelItem(i);
    const QString name = it->text(0);
    const bool show = kw.isEmpty() || name.contains(kw, Qt::CaseInsensitive);
    it->setHidden(!show);
  }
}

void AddTorrentDialog::setAllWanted(bool wanted) {
  if (fileTree_ == nullptr)
    return;
  const Qt::CheckState st = wanted ? Qt::Checked : Qt::Unchecked;
  fileTree_->blockSignals(true);
  for (int i = 0; i < fileTree_->topLevelItemCount(); ++i) {
    auto* it = fileTree_->topLevelItem(i);
    it->setCheckState(2, st);
  }
  fileTree_->blockSignals(false);
  rebuildStats();
}

void AddTorrentDialog::rebuildStats() {
  if (fileTree_ == nullptr)
    return;
  qint64 selected = 0;
  for (int i = 0; i < fileTree_->topLevelItemCount(); ++i) {
    auto* it = fileTree_->topLevelItem(i);
    const int idx = it->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && static_cast<size_t>(idx) < fileSizes_.size() &&
        it->checkState(2) == Qt::Checked) {
      selected += fileSizes_[static_cast<size_t>(idx)];
    }
  }
  if (selectedSizeLabel_ != nullptr) {
    selectedSizeLabel_->setText(
        QStringLiteral("已勾选：%1 / %2").arg(formatBytes(selected), formatBytes(totalBytes_)));
  }
  if (infoSize_ != nullptr) {
    infoSize_->setText(
        QStringLiteral("%1（已勾选：%2）").arg(formatBytes(totalBytes_), formatBytes(selected)));
  }
}

AddTorrentDialog::Result AddTorrentDialog::result() const {
  Result r;
  r.name = torrentName_;
  r.savePath = savePathEdit_ != nullptr ? savePathEdit_->text().trimmed() : QString();
  r.useIncompletePath = useIncompleteCheck_ != nullptr && useIncompleteCheck_->isChecked();
  r.incompletePath =
      incompletePathEdit_ != nullptr ? incompletePathEdit_->text().trimmed() : QString();
  r.category = categoryEdit_ != nullptr ? categoryEdit_->text().trimmed() : QString();
  r.tagsCsv = tagsEdit_ != nullptr ? tagsEdit_->text().trimmed() : QString();
  r.layout = layoutBox_ != nullptr ? static_cast<ContentLayout>(layoutBox_->currentData().toInt())
                                   : ContentLayout::kOriginal;
  r.startTorrent = startCheck_ != nullptr && startCheck_->isChecked();
  r.stopCondition = stopConditionBox_ != nullptr ? stopConditionBox_->currentData().toInt() : 0;
  r.sequentialDownload = sequentialCheck_ != nullptr && sequentialCheck_->isChecked();
  r.firstLastPieces = firstLastCheck_ != nullptr && firstLastCheck_->isChecked();
  r.skipHashCheck = skipHashCheck_ != nullptr && skipHashCheck_->isChecked();
  r.addToTopQueue = addTopQueueCheck_ != nullptr && addTopQueueCheck_->isChecked();
  r.totalBytes = totalBytes_;

  if (fileTree_ != nullptr) {
    r.fileWanted.assign(fileTree_->topLevelItemCount(), 1);
    qint64 selected = 0;
    for (int i = 0; i < fileTree_->topLevelItemCount(); ++i) {
      auto* it = fileTree_->topLevelItem(i);
      const int idx = it->data(0, Qt::UserRole).toInt();
      const bool wanted = it->checkState(2) == Qt::Checked;
      if (idx >= 0 && idx < static_cast<int>(r.fileWanted.size())) {
        r.fileWanted[static_cast<size_t>(idx)] = wanted ? 1 : 0;
      }
      if (wanted && idx >= 0 && static_cast<size_t>(idx) < fileSizes_.size()) {
        selected += fileSizes_[static_cast<size_t>(idx)];
      }
    }
    r.selectedBytes = selected;
  }

  return r;
}

}  // namespace pfd::ui
