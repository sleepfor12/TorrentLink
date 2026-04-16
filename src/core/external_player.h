#ifndef PFD_CORE_EXTERNAL_PLAYER_H
#define PFD_CORE_EXTERNAL_PLAYER_H

#include <QtCore/QString>

namespace pfd::core {

class ExternalPlayer {
public:
  enum class Result { Ok, FileNotFound, LaunchFailed };

  static Result openWithDefault(const QString& file_path);
  static Result openWithCommand(const QString& command, const QString& file_path);
  static Result openFolder(const QString& path);
};

}  // namespace pfd::core

#endif  // PFD_CORE_EXTERNAL_PLAYER_H
