#ifndef PFD_CORE_TORRENT_CREATOR_H
#define PFD_CORE_TORRENT_CREATOR_H

#include <QtCore/QString>

#include <functional>

namespace pfd::core {

struct CreateTorrentRequest {
  QString source_path;
  QString output_torrent_path;
  QString trackers_text;
  QString web_seeds_text;
  QString comment;
  QString source;
  int piece_size_bytes{0};  // 0=auto
  bool private_torrent{false};
};

struct CreateTorrentResult {
  bool ok{false};
  QString error;
};

class TorrentCreator {
public:
  using ProgressCallback = std::function<void(int)>;

  [[nodiscard]] static CreateTorrentResult create(const CreateTorrentRequest& request,
                                                  ProgressCallback on_progress = nullptr);
};

}  // namespace pfd::core

#endif  // PFD_CORE_TORRENT_CREATOR_H
