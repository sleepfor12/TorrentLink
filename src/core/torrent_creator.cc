#include "core/torrent_creator.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/error_code.hpp>

#include <vector>

namespace pfd::core {

CreateTorrentResult TorrentCreator::create(const CreateTorrentRequest& request,
                                           ProgressCallback on_progress) {
  const QString sourcePath = request.source_path.trimmed();
  const QString outputPath = request.output_torrent_path.trimmed();
  if (sourcePath.isEmpty() || outputPath.isEmpty()) {
    return {false, QStringLiteral("源路径或输出路径为空。")};
  }
  const QFileInfo sourceInfo(sourcePath);
  if (!sourceInfo.exists()) {
    return {false, QStringLiteral("源路径不存在：%1").arg(sourcePath)};
  }

  libtorrent::file_storage fs;
  libtorrent::add_files(fs, sourcePath.toStdString());
  if (fs.num_files() <= 0) {
    return {false, QStringLiteral("源路径中没有可用于创建 torrent 的文件。")};
  }

  libtorrent::error_code ec;
  libtorrent::create_torrent torrent(fs,
                                     request.piece_size_bytes > 0 ? request.piece_size_bytes : 0);
  if (request.private_torrent) {
    torrent.set_priv(true);
  }
  if (!request.comment.trimmed().isEmpty()) {
    const QByteArray commentUtf8 = request.comment.trimmed().toUtf8();
    torrent.set_comment(commentUtf8.constData());
  }

  int trackerTier = 0;
  for (const QString& line : request.trackers_text.split('\n', Qt::SkipEmptyParts)) {
    const QString tracker = line.trimmed();
    if (!tracker.isEmpty()) {
      torrent.add_tracker(tracker.toStdString(), trackerTier++);
    }
  }
  for (const QString& line : request.web_seeds_text.split('\n', Qt::SkipEmptyParts)) {
    const QString seed = line.trimmed();
    if (!seed.isEmpty()) {
      torrent.add_url_seed(seed.toStdString());
    }
  }

  const QString hashRoot =
      sourceInfo.isDir() ? sourceInfo.absoluteFilePath() : sourceInfo.absolutePath();
  libtorrent::set_piece_hashes(
      torrent, hashRoot.toStdString(),
      [&](int progress) {
        if (on_progress) {
          on_progress(progress);
        }
      },
      ec);
  if (ec) {
    return {false,
            QStringLiteral("计算分块哈希失败：%1").arg(QString::fromStdString(ec.message()))};
  }

  const libtorrent::entry generated = torrent.generate();
  std::vector<char> encoded;
  encoded.reserve(2 * 1024 * 1024);
  libtorrent::bencode(std::back_inserter(encoded), generated);

  QFile file(outputPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return {false, QStringLiteral("写入文件失败：%1").arg(outputPath)};
  }
  if (file.write(encoded.data(), static_cast<qint64>(encoded.size())) !=
      static_cast<qint64>(encoded.size())) {
    return {false, QStringLiteral("写入 torrent 内容失败：%1").arg(outputPath)};
  }
  file.close();
  if (on_progress) {
    on_progress(100);
  }
  return {true, {}};
}

}  // namespace pfd::core
