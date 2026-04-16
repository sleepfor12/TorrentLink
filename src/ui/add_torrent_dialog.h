#ifndef PFD_UI_ADD_TORRENT_DIALOG_H
#define PFD_UI_ADD_TORRENT_DIALOG_H

#include <QtWidgets/QDialog>

#include <vector>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace pfd::ui {

class AddTorrentDialog : public QDialog {
  Q_OBJECT

public:
  enum class ContentLayout {
    kOriginal = 0,
    kCreateSubfolder,
    kNoSubfolder,
  };

  struct Result {
    QString name;
    QString savePath;
    bool useIncompletePath{false};
    QString incompletePath;

    QString category;
    QString tagsCsv;
    ContentLayout layout{ContentLayout::kOriginal};

    bool startTorrent{true};
    int stopCondition{0};  // 0=无 1=文件已被检查（后续落地）
    bool sequentialDownload{false};
    bool firstLastPieces{false};
    bool skipHashCheck{false};
    bool addToTopQueue{false};

    qint64 selectedBytes{0};
    qint64 totalBytes{0};

    // 每个文件 0=不下载 1=下载（后续会映射为 libtorrent priorities）
    std::vector<int> fileWanted;
  };

  struct MagnetInput {
    QString name;
    qint64 totalBytes{0};
    qint64 creationDate{0};
    QString infoHashV1;
    QString infoHashV2;
    QString comment;
    std::vector<QString> filePaths;
    std::vector<qint64> fileSizes;
  };

  // 从 .torrent 文件打开（第一阶段先实现这个入口）
  static std::optional<Result> runForTorrentFile(QWidget* parent, const QString& torrentFilePath,
                                                 const QString& defaultSavePath);
  static std::optional<Result> runForMagnetMetadata(QWidget* parent, const MagnetInput& in,
                                                    const QString& defaultSavePath);

private:
  explicit AddTorrentDialog(QWidget* parent);
  bool loadTorrentFile(const QString& torrentFilePath);
  bool loadMagnetMetadata(const MagnetInput& in);
  void applyMetaInfoToUi();
  void rebuildFileTree(const std::vector<QString>& filePaths);
  void buildLayout();
  void bindSignals();
  void rebuildStats();
  void applyFilter(const QString& text);
  void setAllWanted(bool wanted);

  Result result() const;

  QLabel* title_{nullptr};

  // left
  QLabel* infoSize_{nullptr};
  QLabel* infoDate_{nullptr};
  QLabel* infoHashV1_{nullptr};
  QLabel* infoHashV2_{nullptr};
  QLabel* infoComment_{nullptr};

  QLineEdit* savePathEdit_{nullptr};
  QPushButton* browseSavePathBtn_{nullptr};
  QCheckBox* useIncompleteCheck_{nullptr};
  QLineEdit* incompletePathEdit_{nullptr};
  QPushButton* browseIncompleteBtn_{nullptr};

  QLineEdit* categoryEdit_{nullptr};
  QLineEdit* tagsEdit_{nullptr};
  QComboBox* layoutBox_{nullptr};
  QCheckBox* startCheck_{nullptr};
  QComboBox* stopConditionBox_{nullptr};
  QCheckBox* sequentialCheck_{nullptr};
  QCheckBox* firstLastCheck_{nullptr};
  QCheckBox* skipHashCheck_{nullptr};
  QCheckBox* addTopQueueCheck_{nullptr};

  // right
  QLineEdit* filterEdit_{nullptr};
  QPushButton* selectAllBtn_{nullptr};
  QPushButton* selectNoneBtn_{nullptr};
  QTreeWidget* fileTree_{nullptr};
  QLabel* selectedSizeLabel_{nullptr};

  // torrent data
  QString torrentName_;
  qint64 totalBytes_{0};
  qint64 creationDate_{0};
  QString comment_;
  QString hashV1_;
  QString hashV2_;
  std::vector<qint64> fileSizes_;
};

}  // namespace pfd::ui

#endif  // PFD_UI_ADD_TORRENT_DIALOG_H
