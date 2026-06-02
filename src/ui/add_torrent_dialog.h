#ifndef PFD_UI_ADD_TORRENT_DIALOG_H
#define PFD_UI_ADD_TORRENT_DIALOG_H

#include <QtWidgets/QDialog>

#include <functional>
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

  /// 从 magnet URI 解析出的初始展示信息（元数据到达前显示）。
  struct MagnetBootstrap {
    QString displayName;
    QString infoHashV1;
    QString infoHashV2;
  };

  enum class MetadataPollState { kPending, kReady, kFailed };

  struct MetadataPollResult {
    MetadataPollState state{MetadataPollState::kPending};
    MagnetInput input;
  };

  using MetadataPoller = std::function<MetadataPollResult()>;

  // 从 .torrent 文件打开
  static std::optional<Result> runForTorrentFile(QWidget* parent, const QString& torrentFilePath,
                                                 const QString& defaultSavePath);
  /// 元数据已就绪时使用（RSS 等路径）。
  static std::optional<Result> runForMagnetMetadata(QWidget* parent, const MagnetInput& in,
                                                    const QString& defaultSavePath);
  /// 立即弹出对话框，同时通过 poller 在后台检索元数据并刷新界面。
  static std::optional<Result> runForMagnetLinkPending(QWidget* parent,
                                                       const MagnetBootstrap& bootstrap,
                                                       const QString& defaultSavePath,
                                                       const MetadataPoller& poller);

private:
  explicit AddTorrentDialog(QWidget* parent);
  void beginMagnetPending(const MagnetBootstrap& bootstrap, const QString& defaultSavePath);
  void applyMagnetMetadata(const MagnetInput& in);
  void setMetadataFetchFailed();
  void setMetadataControlsEnabled(bool enabled);
  [[nodiscard]] bool isMetadataReady() const {
    return metadataReady_;
  }
  [[nodiscard]] bool metadataFetchFailed() const {
    return metadataFailed_;
  }
  void applyMetaInfoToUi();
  void rebuildFileTree(const std::vector<QString>& filePaths);
  void showMetadataLoadingPlaceholder();
  bool loadTorrentFile(const QString& torrentFilePath);
  bool loadMagnetMetadata(const MagnetInput& in);
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
  QPushButton* acceptBtn_{nullptr};

  bool metadataReady_{false};
  bool metadataFailed_{false};

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
